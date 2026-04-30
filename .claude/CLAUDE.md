# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**RD** is a real-time audio pitch-shifting VST3/Standalone plugin built on JUCE, implementing **TD-PSOLA (Time-Domain Pitch Synchronous Overlap-Add)** grain synthesis. C++20, CMake build system.

## Build Commands

```bash
# Configure (from repo root)
cmake -B BUILD/ -DBUILD_TESTS=ON

# Build
cmake --build BUILD/

# Run tests
cd BUILD && ctest
```

**Debug build flags** (pass to cmake configure step):
- `-DRD_DEBUG_ALL=ON` — enable all debug logging
- `-DRD_DEBUG_PITCH_DETECTION=ON` / `-DRD_DEBUG_GRAIN_CREATION=ON` / `-DRD_DEBUG_GRAIN_PROCESSING=ON`
- `-DRD_DEBUG_SAMPLE_DETAIL=ON` / `-DRD_DEBUG_OUTPUT_STATS=ON` / `-DRD_DEBUG_GAIN_PROCESSING=ON`

Release builds strip all debug code at compile time (zero overhead).

### Build Mode: Plugin vs Static Library

RD auto-detects context in `CMakeLists.txt`:
- **Top-level build** → `BUILD_AS_PLUGIN=ON` → produces VST3 + Standalone targets via `juce_add_plugin`.
- **Included as submodule** → `BUILD_AS_PLUGIN=OFF` → produces static library target named `RD`.

Override explicitly with `-DBUILD_AS_PLUGIN=ON/OFF` if needed.

### Submodule-in-Parent Guardrail

If RD lives inside another project's `SUBMODULES/` directory, CMake will **fatal-error** on any attempt to configure RD standalone from that nested path. The parent project must drive the build. Do not work around this — clone RD as its own repo for standalone work.

### Optional Intel IPP (Windows)

`find_package(IPP)` is attempted on MSVC. If found, links `IPP::ipps/ippcore/ippi/ippcv` and defines `PAMPLEJUCE_IPP=1`. Silently skipped otherwise; build still succeeds.

### Helper Scripts

Python automation in `HELPER_SCRIPTS/`:
- `build_vst3.py`, `build_tests.py`, `rebuild_all.py` — build drivers
- `build_installer.py`, `sign_builds.py`, `sign_installers.py` — packaging/signing
- `release_workflow.py`, `build_and_release_workflow.py` — end-to-end release
- `update_version.py` — invoked by CMake custom target `update_version_header` to regenerate `SOURCE/Util/Version.h` from `VERSION.txt`

## Testing

Framework: **Catch2 v3** (auto-fetched via FetchContent). Test utilities in `TESTS/TEST_UTILS/`, golden reference audio in `TESTS/GOLDEN/`.

Source list for tests is managed in `CMAKE/TESTS.cmake` — add new test files there.

### Test Layout Convention

Processor tests live under `TESTS/PROCESSORS/<PROCESSOR_NAME>/`. Each processor folder has:

- `test_<Processor>.cpp` — behavior/unit tests (tagged `[<Processor>]`).
- `test_<Processor>_DataLogger.cpp` — DataLogger output tests (tagged `[<Processor>][DataLogger]`).
- `OUTPUT/` — gitignored, holds timestamped log artifacts. A `.gitkeep` keeps the directory tracked.

Keep `[DataLogger]` cases out of the main behavior test file — separate file per processor.

Standalone-component tests live in their own top-level test folder: `TESTS/BUFFER_FILLER/`, `TESTS/DATA_LOGGER/`. Use these for tests that aren't tied to a specific processor.

### DataLogger Test Protocol

For any processor inheriting `RD_Processor` / `DataLogger`, the `_DataLogger.cpp` file follows this pattern (see `TESTS/PROCESSORS/GAIN_PROCESSOR/test_GainProcessor_DataLogger.cpp` for canonical example):

1. Build a timestamped `outputDir` under `TESTS/PROCESSORS/<PROCESSOR>/OUTPUT/<TEST CASE NAME>/<timestamp>`. Treat `outputDir` as the **root directory** for the logger.
2. Per `SECTION`, configure the logger then call `startLogging()`:
   ```cpp
   processor.setDataLogRootDirectory (outputDir);
   processor.setDataLogOutputName ("<section name>");
   processor.startLogging();   // sets isLogging=true, deletes outputDir recursively
   ```
   so each section's logs are isolated under `outputDir/<section name>/`.
3. Lifecycle log layout under `getDataLogOutputDirectory()`:

   | Event | Location | Files |
   |-------|----------|-------|
   | `prepareToPlay` | `prepare_to_play/` subdir | `prepare_to_play.csv`, `processor_state.xml` |
   | `processBlock` (start) | output dir root | `input_samples.csv` (appended) |
   | `processBlock` (end)   | output dir root | `output_samples.csv` (appended) |

   `processBlock` does not write `processor_state.xml` — use `createProcessorDataLogFile()` externally for that.

   Per-block CSV row layout (each `processBlock` call appends `2 + numChannels` rows):
   - Row 0: global sample indices (`mLogBlockStartIndex + s` for s in `[0, blockSize)`)
   - Row 1: local sample indices (`0..blockSize-1`)
   - Rows 2..(1+numChannels): per-channel sample values, one row per channel

   `mLogBlockStartIndex` snapshots `mProcessSampleCount` before each block; `mProcessSampleCount` is cumulative since `prepareToPlay` and increments by `blockSize` each block. After N blocks each csv has `(2 + numChannels) * N` rows.

   `_appendBlockSamplesCsv` and `_writeProcessorStateXml` use `juce::FileOutputStream` directly (not `replaceWithText` / `XmlElement::writeTo(File)`) to avoid JUCE's `TemporaryFile` suffix overflowing Windows MAX_PATH on deeply nested test paths.

   Call `stopLogging()` when done.

A `DataLogger`'s output is **always a directory** containing files, never a loose file. The output directory is composed dynamically from `getDataLogParentDirectory()` (which equals `mDataLogRootDirectory` if no parent logger is registered, or `parentLogger->getDataLogOutputDirectory()` otherwise) plus `mDataLogOutputName` (defaults to construction timestamp).

Two ways `processor_state.xml` gets written:
- **Lifecycle logs**: each event subdir (above) contains its own `processor_state.xml` snapshotting the APVTS at the moment of the event.
- **External API**: `createProcessorDataLogFile()` writes `processor_state.xml` directly into `getDataLogOutputDirectory()` (root, not a subdir) for callers that want a one-off snapshot independent of lifecycle.

Lifecycle subdirs are materialized at `logData()` time. Callers should not pre-create them.

`DataLogger` owns a non-owning child registry (`addChild` / `removeChild` / `getNumChildren`). When a parent's `logData()` fires, it cascades to registered children. If the parent's `mIsLogging` is false, `logData()` short-circuits and children are not visited.

Children also hold a non-owning back-pointer to their parent logger (`mParentLogger`, set by `addChild`, cleared by `removeChild`). The child's `getDataLogParentDirectory()` walks up to the parent's current `getDataLogOutputDirectory()` on every call, so child output always nests under the parent's current location and follows parent renames automatically:
`child.getDataLogOutputDirectory() == parent.getDataLogOutputDirectory() / child.getDataLogOutputName()`.

Tests on composite processors must verify children actually log when parent logs, and that child paths land inside the parent's output directory.

### Running a single test

From `BUILD/` after building:

```bash
./Tests --list-tests              # enumerate all test cases
./Tests "[tag]"                   # run all tests with given tag
./Tests "exact test case name"    # run one case
ctest -R <regex>                  # ctest-level filter
```

## Architecture

### Signal Flow

```
Audio In → CircularBuffer → PitchManager (detect) → Granulator (synthesize) → Audio Out
```

The `CircularBuffer` is the **single source of truth** for all audio. Everything reads from it using absolute sample indices; it never discards audio until overwritten by new input.

### Processor Container

`RD_ProcessorSwapper` hosts all processors inside a `juce::AudioProcessorGraph`. Currently supports two processor slots:
- **GainProcessor** — simple gain adjustment
- **GrainShifterProcessor** — TD-PSOLA pitch shifter (main engine)

`Fade` handles crossfading when switching between processors.

### GrainShifterProcessor Internals

| Component | Role |
|-----------|------|
| `CircularBuffer` | Ring buffer storing all incoming audio |
| `PitchManager` | Coordinates pitch detection; owns `FFT_PitchDetector`, `PitchMarker`, `SynthMarker` |
| `FFT_PitchDetector` | FFT-based autocorrelation pitch detection (the active detector used for grain shifting) |
| `YIN_PitchDetector` | YIN/CMND reference implementation — not wired into `PitchManager`. Do not swap in without benchmarking. |
| `PitchMarker` | Tracks detected pitch mark positions in the circular buffer (30-second FIFO) |
| `SynthMarker` | Tracks synthesis event positions relative to pitch marks |
| `Granulator` | Pre-allocated pool of `Grain` objects; voice-pool pattern (reuses finished grains) |
| `Grain` | Single windowed grain using TD-PSOLA technique; applies Hann/Tukey window and overlap-adds into output |
| `Window` | Generates Hann, Tukey, and other window functions |

### Parameter Wiring (APVTS)

Each processor owns its own `mAPVTS` via override, rather than sharing a single `mBaseAPVTS` on `RD_Processor`. When adding or touching parameters on a processor subclass, define them on that subclass's APVTS — do not route through the base class.

### RD_Processor Template Method (`processBlock` / `doProcessBlock`)

`RD_Processor::processBlock` is `final` — child classes **cannot** override it. Instead, override `virtual void doProcessBlock(buffer, midi)`. The base `processBlock` wraps the child call with shared concerns:

1. Snapshot `mProcessSampleCount` into `mLogBlockStartIndex`.
2. If `getIsLogging()`, copy buffer into `mLogBuffer` and fire `kProcessBlockStart` lifecycle log → appends a `2 + numChannels`-row block to `input_samples.csv` at the output dir root.
3. Call `doProcessBlock(buffer, midi)` — child does its DSP work in place on `buffer`.
4. If `getIsLogging()`, copy post-DSP buffer into `mLogBuffer` and fire `kProcessBlockEnd` lifecycle log → appends to `output_samples.csv` at the output dir root.
5. Increment `mProcessSampleCount` by `buffer.getNumSamples()`.

`prepareToPlay` is also `final`; override `doPrepareToPlay`. The base fires `kPreparedToPlay` → writes `prepare_to_play/{prepare_to_play.csv, processor_state.xml}`.

`startLogging()` deletes the entire output directory recursively for a clean slate. `stopLogging()` flips the flag. See DataLogger Test Protocol above for the per-event subdirectory layout.

When adding a new processor, override `doProcessBlock` / `doPrepareToPlay`, not the `final` versions. `RD_ProcessorSwapper` also inherits `RD_Processor` and follows the same convention — its graph dispatch lives in `doProcessBlock`.

### Real-Time Safety

The hot path does **no dynamic allocation**. Grain pools are pre-allocated. The `CircularBuffer` uses absolute indices. All real-time-safe patterns are intentional — do not introduce heap allocations in `processBlock` paths.

## Debug Logging System

Two-layer system:

**Compile-time** (CMake flags above): controls whether debug code exists in the binary.

**Runtime** (when compiled in): toggle via `DebugLogger` methods — `enableGrainCreation()`, `enablePitchDetection()`, `enableAll()`, etc.

**Macros** (in `SOURCE/Util/DebugLog.h`):
- `LOG_ONCE()` — log first N times
- `LOG_COUNTED()` — log while counter < limit  
- `LOG_ALWAYS()` — log every call (when enabled)
- `LOG_GRAIN_CREATION()`, `LOG_PITCH_DETECTION()`, etc. — category-specific helpers

## Key File Locations

| What | Where |
|------|-------|
| Source list | `CMAKE/SOURCES.cmake` |
| Test list | `CMAKE/TESTS.cmake` |
| Plugin container | `SOURCE/PROCESSORS/RD_ProcessorSwapper.h/cpp` |
| Grain shifter (TD-PSOLA) | `SOURCE/PROCESSORS/GRAIN/GrainShifterProcessor.h/cpp` |
| Circular buffer | `SOURCE/CircularBuffer.h/cpp` |
| Pitch detection | `SOURCE/PITCH/` |
| Granulator | `SOURCE/PROCESSORS/GRAIN/Granulator.h/cpp` |
| DataLogger (mixin + child registry) | `SOURCE/DATA_LOGGER/DataLogger.h/cpp` |
| BufferFiller (WAV → AudioBuffer) | `SOURCE/BUFFER_FILLER/BufferFiller.h/cpp` |
| Debug macros | `SOURCE/Util/DebugLog.h` |
| JUCE includes | `SOURCE/Util/Juce_Header.h` (include this, not JUCE directly) |
| Version | `VERSION.txt` (auto-generates `SOURCE/Util/Version.h`) |
