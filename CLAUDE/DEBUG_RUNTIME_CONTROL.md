# Debug Logger Runtime Control

## Hybrid Design

The debug logging system uses a **hybrid approach**:

1. **Compile-time guards** (CMake flags) - Calls to DebugLogger compile out completely in release builds
2. **Runtime flags** (class members) - When compiled in, logging can be toggled at runtime

This gives you:
- ✅ Zero overhead in release builds (code doesn't exist)
- ✅ Dynamic control in debug builds (perfect for testing)

## Compile-Time Control (CMake)

Controls whether debug calls exist at all:

```bash
# Enable categories at compile time
cmake -DRD_DEBUG_GRAIN_CREATION=ON BUILD
cmake -DRD_DEBUG_PITCH_DETECTION=ON BUILD

# Release builds - all debug code removed
cmake BUILD
cmake --build BUILD --config Release
```

If `RD_DEBUG_GRAIN_CREATION=OFF`, this code doesn't exist:
```cpp
#if RD_DEBUG_GRAIN_CREATION  // Compile-time guard
    DebugLogger::logGrainCreation(...);
#endif
```

## Runtime Control (C++ API)

When debug calls are compiled in, you can toggle them at runtime:

### Enable/Disable Categories

```cpp
// Disable specific category
DebugLogger::enableGrainCreation(false);

// Re-enable it
DebugLogger::enableGrainCreation(true);

// Disable all logging
DebugLogger::enableAll(false);

// Enable only pitch detection
DebugLogger::enableAll(false);
DebugLogger::enablePitchDetection(true);
```

### Query Status

```cpp
if (DebugLogger::isGrainCreationEnabled())
{
    // Do something
}
```

## Use Cases

### Testing: Enable Logging for One Test

```cpp
TEST_CASE("Debug grain creation logic")
{
    // Start clean - disable all debug output
    DebugLogger::enableAll(false);

    // Enable only grain creation logging for this test
    DebugLogger::enableGrainCreation(true);

    // Run test - will only see grain creation logs
    GranulatorProcessor processor;
    processor.processBlock(...);

    // Restore defaults
    DebugLogger::enableAll(true);
}
```

### Testing: Temporarily Suppress Noisy Logs

```cpp
TEST_CASE("Test with quiet output")
{
    // Disable noisy sample detail logging
    DebugLogger::enableSampleDetail(false);

    // Test runs with less output
    runLongTest();

    // Re-enable
    DebugLogger::enableSampleDetail(true);
}
```

### Development: Focus on One Subsystem

```cpp
int main()
{
    // Only see pitch detection logs
    DebugLogger::enableAll(false);
    DebugLogger::enablePitchDetection(true);

    // Run application
    runApp();
}
```

## Available Categories

| Category | Enable Method | CMake Flag |
|----------|---------------|------------|
| Pitch Detection | `enablePitchDetection()` | `RD_DEBUG_PITCH_DETECTION` |
| Grain Creation | `enableGrainCreation()` | `RD_DEBUG_GRAIN_CREATION` |
| Grain Processing | `enableGrainProcessing()` | `RD_DEBUG_GRAIN_PROCESSING` |
| Sample Detail | `enableSampleDetail()` | `RD_DEBUG_SAMPLE_DETAIL` |
| Output Stats | `enableOutputStats()` | `RD_DEBUG_OUTPUT_STATS` |
| Gain Processing | `enableGainProcessing()` | `RD_DEBUG_GAIN_PROCESSING` |

## Default State

All categories default to **enabled** (when compiled in). This means:

- If `RD_DEBUG_GRAIN_CREATION=ON` at compile time, grain creation logs appear by default
- You must explicitly disable them with `enableGrainCreation(false)` to suppress output

## Performance

### Debug Build (Categories Compiled In)

```cpp
void DebugLogger::logGrainCreation(...)
{
    if (!sGrainCreationEnabled)  // Single boolean check
        return;

    std::cout << ...;  // String formatting and output
}
```

**Cost when disabled**: One boolean check per call
**Cost when enabled**: Boolean check + formatting + I/O

### Release Build (Categories Compiled Out)

```cpp
#if RD_DEBUG_GRAIN_CREATION  // = 0, entire block removed
    DebugLogger::logGrainCreation(...);
#endif
```

**Cost**: Zero - code doesn't exist in binary

## Examples in Practice

### Example 1: Test Suite with Selective Logging

```cpp
// Test setup - silence everything by default
DebugLogger::enableAll(false);

TEST_CASE("Pitch detection accuracy")
{
    // Enable only pitch detection for this test
    DebugLogger::enablePitchDetection(true);

    // Test runs with focused output
    REQUIRE(detector.process(buffer) == expectedPeriod);

    DebugLogger::enablePitchDetection(false);
}

TEST_CASE("Grain creation timing")
{
    // Different test, different logging needs
    DebugLogger::enableGrainCreation(true);
    DebugLogger::enableGrainProcessing(true);

    // Test with grain-focused output
    REQUIRE(grains.size() == expectedCount);

    DebugLogger::enableAll(false);
}
```

### Example 2: Development Workflow

```bash
# Day 1: Working on pitch detection
cmake -DRD_DEBUG_PITCH_DETECTION=ON BUILD

# Day 2: Working on granulator
cmake -DRD_DEBUG_GRAIN_CREATION=ON -DRD_DEBUG_GRAIN_PROCESSING=ON BUILD

# Production release
cmake BUILD
cmake --build BUILD --config Release
```

Runtime control lets you further refine without recompiling:
```cpp
// In your debugging session
DebugLogger::enableGrainProcessing(false);  // Too noisy
// Continue debugging with just grain creation logs
```

## Summary

**Compile-time flags** = Control what CAN be logged (binary size, release safety)
**Runtime flags** = Control what IS logged (testing flexibility, development workflow)

Best of both worlds! 🎉
