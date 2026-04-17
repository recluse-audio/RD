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

## Testing

Framework: **Catch2 v3** (auto-fetched via FetchContent). Test files live in `TESTS/`, test utilities in `TESTS/TEST_UTILS/`, golden reference audio in `TESTS/GOLDEN/`.

Source list for tests is managed in `CMAKE/TESTS.cmake` — add new test files there.

## Architecture

### Signal Flow

```
Audio In → CircularBuffer → PitchManager (detect) → TD_Granulator (synthesize) → Audio Out
```

The `CircularBuffer` is the **single source of truth** for all audio. Everything reads from it using absolute sample indices; it never discards audio until overwritten by new input.

### Processor Container

`RD_ProcessorSwapper` hosts all processors inside a `juce::AudioProcessorGraph`. Currently supports two processor slots:
- **GainProcessor** — simple gain adjustment
- **TDPSOLA_Processor** — pitch shifter (main engine)

`Fade` handles crossfading when switching between processors.

### TDPSOLA_Processor Internals

| Component | Role |
|-----------|------|
| `CircularBuffer` | Ring buffer storing all incoming audio |
| `PitchManager` | Coordinates pitch detection; owns `TD_PitchDetector`, `PitchMarker`, `SynthMarker` |
| `TD_PitchDetector` | Runs time-domain pitch detection algorithm |
| `PitchMarker` | Tracks detected pitch mark positions in the circular buffer (30-second FIFO) |
| `SynthMarker` | Tracks synthesis event positions relative to pitch marks |
| `TD_Granulator` | Pre-allocated pool of `TD_Grain` objects; voice-pool pattern (reuses finished grains) |
| `TD_Grain` | Single windowed grain; applies Hann window and overlap-adds into output |
| `Window` | Generates Hann and other window functions |

### Real-Time Safety

The hot path does **no dynamic allocation**. Grain pools are pre-allocated. The `CircularBuffer` uses absolute indices. All real-time-safe patterns are intentional — do not introduce heap allocations in `processBlock` paths.

## Debug Logging System

Two-layer system documented in `INSTRUCTIONS/DEBUG_LOGGING_GUIDE.md` and `INSTRUCTIONS/DEBUG_RUNTIME_CONTROL.md`:

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
| Pitch shifter | `SOURCE/PROCESSORS/TDPSOLA/TDPSOLA_Processor.h/cpp` |
| Circular buffer | `SOURCE/CircularBuffer.h/cpp` |
| Pitch detection | `SOURCE/PITCH/` |
| Granulator | `SOURCE/PROCESSORS/TDPSOLA/TD_Granulator.h/cpp` |
| Debug macros | `SOURCE/Util/DebugLog.h` |
| JUCE includes | `SOURCE/Util/Juce_Header.h` (include this, not JUCE directly) |
| Version | `VERSION.txt` (auto-generates `SOURCE/Util/Version.h`) |
