# Plan: Fix Bug 2 — Pitch-detector period instability

## Context

Diagnosis from comparing `synthesis_grains.csv` A (this project) vs B (RD_PSOLA reference at 1.0 transpose) revealed three bugs. Bug 2 — wild period jumps from `FFT_PitchDetector` (43↔284, 79↔317, 339↔58, 48↔447) — drives most downstream artifacts because synth spacing and grain windowing both feed off `mCurrentPeriod`. Fixing pitch first eliminates the upstream cause of Bugs 1 and 3 in many cases.

## Root cause (verified in code)

`SUBMODULES/RD/SOURCE/PITCH/FFT_PitchDetector.cpp`:
- **Line 100–113**: peak picked as raw `argmax(autocorr[i])` over `[mMinPeriod, mMaxPeriod]`. No threshold, no octave check, no continuity bias.
- **Line 60 / `mThreshold` declared in `.h:73`**: threshold field exists, **never read**. Dead code.
- **Line 84**: `fftData[0] = 0` zeros DC — also wipes the lag-0 normalization reference, so any threshold scheme has to compute reference energy independently.

`SUBMODULES/RD/SOURCE/PITCH/PitchManager.cpp`:
- **Line 100**: `mCurrentPeriod` taken raw from detector each call. No history, no smoothing.
- **Line 119**: `windowSize / mCurrentPeriod` — divides by detected period without guarding negative/zero. Already a potential div-by-near-zero crash if detector returns small spurious value, and an int truncation that yields garbage iteration count when period is bogus-large.

Failure modes producing observed jumps:
- **Octave-down (harmonic of true period wins)**: autocorr peaks at 2T, 3T, kT scale with k for periodic signals; long windows let kT exceed T. Drives 43 → 284 type spikes (~ 6.6× ≈ near a sub-multiple).
- **Octave-up (harmonic capture)**: when fundamental is weak relative to harmonic, peak at T/2 wins.
- **Noise / silent regions**: argmax returns whatever lag has marginally largest random value. Threshold gate currently absent → emits "pitch" for unpitched audio.

## Incremental fix sequence

Each step is a self-contained, independently testable change. Stop after each, re-render the test file, diff `synthesis_grains.csv`, listen to output. Do not advance until the prior step is verified.

### Step 1 — handle invalid period downstream (defensive, no algorithmic change)

**Problem:** `FFT_PitchDetector::process` already returns `-1` when window is too small (`.cpp:53–56`). `PitchManager::detect` at `.cpp:119` then computes `windowSize / mCurrentPeriod` with a negative — silent corruption. Future steps will return `-1` in more cases (low-confidence, no peak above threshold), so wire the guard now before changing detection.

**Change:** in `PitchManager::detect` (`PitchManager.cpp:100`), after the `mPitchDetector.process` call, if `mCurrentPeriod <= 0`:
- skip pitch-mark generation loop entirely;
- skip synth-mark generation;
- still fire the `kDetect` log row (so we can see the -1 in `detect_log.csv`);
- return `mCurrentPeriod` immediately.

Document that callers must treat `-1` as "no pitch this hop, hold prior state."

**Verify:** existing tests still pass; `detect_log.csv` shows occasional `-1` in silent regions; no NaN / runaway grain counts.

### Step 2 — wire `mThreshold` (confidence gate)

**Problem:** detector emits a period even from noise / silence. `mThreshold` field is unused.

**Change in `FFT_PitchDetector::process` (`.cpp:99–114`):**
1. Compute reference energy *before* zeroing DC: store `referenceEnergy = fftData[0]` (the lag-0 autocorrelation = sum of squares ≈ signal energy after FFT→IFFT round trip). Easier alternative: compute `referenceEnergy = max(autocorr[0], small epsilon)` from the inverse-FFT output **before** the peak search by reading `fftData[0]`. Simplest and correct: zero DC AFTER computing reference, OR compute reference as `Σ signal[i]²` directly from the input buffer at the top of `process`.
2. Compute `normalizedPeak = peakValue / referenceEnergy`.
3. If `normalizedPeak < mThreshold`, set `mCurrentPeriod = -1.0f` and return.

Default threshold stays `kDefaultThreshold = 0.15f`. Expose tunability through existing `setThreshold`.

**Verify:** silent regions of input now produce `-1` periods (visible in `detect_log.csv`). Voiced regions unchanged. Step 1's downstream guard absorbs the new `-1`s without breakage. Listen: chunks of unvoiced material that previously granulated into noise are now silent.

### Step 3 — octave-error correction (continuity bias)

**Problem:** raw argmax flips between fundamental and harmonics. Drives 43↔284 jumps.

**Change in `FFT_PitchDetector::process`:**
1. Track `mLastValidPeriod` (member field, default -1, updated only when a valid period is emitted).
2. After raw peak found at `peakIndex`:
   - If `mLastValidPeriod > 0`, evaluate autocorr at `prev`, `2*prev`, `prev/2` (clamped to bounds).
   - Score each candidate as `autocorr[lag] * continuityWeight(lag)` where `continuityWeight = 1.0` for `lag == prev`, `0.85` for `2*prev`, `0.85` for `prev/2`, `0.7` for raw `peakIndex` if outside that family.
   - Pick highest-scored candidate as `mCurrentPeriod`.
3. If `|log2(peakIndex / prev)| > 1.0` (more than one octave jump) AND raw peak does not dominate continuity candidates by >2×, prefer the continuity candidate.

This is hysteresis — protects steady pitched material from one-frame harmonic flips, still allows real pitch changes when raw peak strongly dominates.

Reset `mLastValidPeriod` to `-1` when detector emits `-1` from Step 2 (lost track → next valid detection starts fresh, no stale anchor).

**Verify:** rerun on test input; `detect_log.csv` should show no more 6×–9× jumps between adjacent valid frames. `synthesis_grains.csv` rows-with-extreme-ratio count drops sharply (currently 40+; target near zero).

### Step 4 — median-of-3 smoothing in `PitchManager`

**Problem:** even with octave protection, single-frame outliers can leak (e.g. transient onset fooling autocorr).

**Change in `PitchManager`:**
1. Add small ring `std::array<float, 3> mPeriodHistory` initialized to -1.
2. After `mPitchDetector.process` returns valid period, push into ring; emit median of the three valid entries (ignore -1 entries; with <3 valid, emit raw).
3. Use the smoothed value for `mCurrentPeriod` and for downstream pitch/synth marker calls.

Do **not** smooth during voiced→unvoiced or unvoiced→voiced transitions: if the new sample is `-1`, do not emit smoothed prior value (would create phantom pitch in silence). If two of three are `-1`, emit `-1`.

**Verify:** A/B audio: lingering single-frame pitch ticks gone. Confirm transient response (onsets, pitch slides) not over-smoothed — listen for "lazy" pitch tracking that lags the input. If lagging, drop to median-of-2 or skip Step 4.

## Stop conditions / acceptance per step

| Step | Pass criterion |
|---|---|
| 1 | No crashes; `-1` periods routed cleanly; existing tests green |
| 2 | Unvoiced regions emit `-1`; voiced regions unchanged in CSV |
| 3 | Zero rows with `|log2(syn_per/src_per)| > 1` between adjacent voiced frames |
| 4 | No regression in pitch tracking responsiveness vs Step 3 audio |

## Critical files

- `SUBMODULES/RD/SOURCE/PITCH/FFT_PitchDetector.h` — declare `mLastValidPeriod`.
- `SUBMODULES/RD/SOURCE/PITCH/FFT_PitchDetector.cpp` — Steps 2 + 3.
- `SUBMODULES/RD/SOURCE/PITCH/PitchManager.h` — add `mPeriodHistory`.
- `SUBMODULES/RD/SOURCE/PITCH/PitchManager.cpp` — Steps 1 + 4.

## Verification harness (Step-agnostic)

User drives all builds/tests (per memory). After each step:
1. User builds RD + AudioFileTransformer.
2. User runs the same offline transform that produced current csv A.
3. Compare new `synthesis_grains.csv` vs current csv A:
   - Count rows where `|log2(synthesis_period / source_period)| > 1` (target: trends to 0).
   - Count consecutive duplicate `source_analysis_id` (Bug 3 — secondary indicator; should also drop as fewer giant pitch-mark ranges get emitted).
4. Compare new `detect_log.csv` for jump count vs prior step.
5. A/B listen to rendered WAV against prior step's WAV.

Reference invariants from csv B (1.0 transpose, healthy):
- `synthesis_period == source_period` per row.
- `duration_samples == 2 * synthesis_period`.
- `source_analysis_id` strictly +1 per row.

## Out of scope (reserved for separate plans)

- Bug 1 (window-length formula) — addressed after Bug 2 to see how much it residually matters.
- Bug 3 (SynthMarker pitch-mark selection) — addressed last.
