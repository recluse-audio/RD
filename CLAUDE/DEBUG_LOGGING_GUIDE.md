# Debug Logging System

## Overview

The RD submodule now uses a compile-time debug logging system controlled via CMake options. This replaces ad-hoc `static int counter` patterns with organized, configurable macros.

## CMake Options

Enable debug categories via CMake:

```bash
# Enable all debug logging
cmake -DRD_DEBUG_ALL=ON ..

# Enable specific categories
cmake -DRD_DEBUG_GRAIN_CREATION=ON ..
cmake -DRD_DEBUG_PITCH_DETECTION=ON ..
cmake -DRD_DEBUG_GRAIN_PROCESSING=ON ..
cmake -DRD_DEBUG_SAMPLE_DETAIL=ON ..
cmake -DRD_DEBUG_OUTPUT_STATS=ON ..
cmake -DRD_DEBUG_GAIN_PROCESSING=ON ..

# Combine multiple
cmake -DRD_DEBUG_GRAIN_CREATION=ON -DRD_DEBUG_PITCH_DETECTION=ON ..
```

## Available Macros

### 1. `LOG_ONCE(category, max_count, code)`
Logs only the first N times (uses internal static counter).

**Before:**
```cpp
static int grainCounter = 0;
if (grainCounter < 3) {
    std::cout << "Grain #" << grainCounter << std::endl;
    grainCounter++;
}
```

**After:**
```cpp
LOG_GRAIN_CREATION(3, {
    static int grainCounter = 0;
    std::cout << "Grain #" << grainCounter << std::endl;
    grainCounter++;
});
```

### 2. `LOG_COUNTED(category, max_count, counter_var, code)`
Logs while external counter < max_count.

**Before:**
```cpp
static int blockCounter = 0;
if (blockCounter < 25) {
    std::cout << "Block #" << blockCounter << std::endl;
}
blockCounter++;
```

**After:**
```cpp
static int blockCounter = 0;
LOG_PITCH_DETECTION(25, blockCounter, {
    std::cout << "Block #" << blockCounter << std::endl;
});
blockCounter++;
```

### 3. `LOG_ALWAYS(category, code)`
Logs every time (when category enabled).

**Before:**
```cpp
#ifdef DEBUG_GRAINS
std::cout << "Processing grain" << std::endl;
#endif
```

**After:**
```cpp
LOG_ALWAYS(RD_DEBUG_GRAIN_PROCESSING, {
    std::cout << "Processing grain" << std::endl;
});
```

### 4. `LOG_CONDITIONAL(category, condition, code)`
Logs when category enabled AND condition true.

**Before:**
```cpp
#ifdef DEBUG_SAMPLES
if (sampleIndex == 0) {
    std::cout << "First sample" << std::endl;
}
#endif
```

**After:**
```cpp
LOG_SAMPLE_DETAIL(sampleIndex == 0, {
    std::cout << "First sample" << std::endl;
});
```

## Convenience Macros

| Macro | Equivalent To |
|-------|---------------|
| `LOG_GRAIN_CREATION(max, code)` | `LOG_ONCE(RD_DEBUG_GRAIN_CREATION, max, code)` |
| `LOG_GRAIN_PROCESSING(max, code)` | `LOG_ONCE(RD_DEBUG_GRAIN_PROCESSING, max, code)` |
| `LOG_PITCH_DETECTION(max, ctr, code)` | `LOG_COUNTED(RD_DEBUG_PITCH_DETECTION, max, ctr, code)` |
| `LOG_SAMPLE_DETAIL(cond, code)` | `LOG_CONDITIONAL(RD_DEBUG_SAMPLE_DETAIL, cond, code)` |
| `LOG_OUTPUT_STATS(max, code)` | `LOG_ONCE(RD_DEBUG_OUTPUT_STATS, max, code)` |
| `LOG_GAIN_PROCESSING(max, code)` | `LOG_ONCE(RD_DEBUG_GAIN_PROCESSING, max, code)` |

## Migration Guide

### Files with Debug Logging

1. **Granulator.cpp**
   - `grainCounter` (line 176) → `LOG_GRAIN_CREATION`
   - `callCounter` (line 271) → `LOG_GRAIN_PROCESSING`
   - `sampleLogCounter` (line 328) → `LOG_SAMPLE_DETAIL`
   - `outputLogCount` (line 377) → `LOG_OUTPUT_STATS`

2. **GranulatorProcessor.cpp**
   - `blockCounter` (line 203) → `LOG_PITCH_DETECTION` ✅ DONE
   - `gainLogCount` (line 246) → `LOG_GAIN_PROCESSING` ✅ DONE

### Conversion Steps

For each file:

1. **Add header:**
   ```cpp
   #include "Util/DebugLog.h"  // or "../../Util/DebugLog.h"
   ```

2. **Find pattern:**
   ```cpp
   static int xxxCounter = 0;
   if (xxxCounter < N) {
       // logging code
       xxxCounter++;
   }
   ```

3. **Replace with macro:**
   ```cpp
   LOG_CATEGORY(N, {
       static int xxxCounter = 0;
       // logging code
       xxxCounter++;
   });
   ```

4. **Remove disabling hacks:**
   ```cpp
   // OLD: static int grainCounter = 999;  // DISABLED
   // NEW: Just set RD_DEBUG_GRAIN_CREATION=OFF in CMake
   ```

## Build Examples

```bash
# Development build with all debug logging
cd BUILD
cmake -DRD_DEBUG_ALL=ON ..
cmake --build .

# Release build (no debug logging)
cmake ..
cmake --build . --config Release

# Debug specific subsystem
cmake -DRD_DEBUG_PITCH_DETECTION=ON ..
cmake --build .
```

## Benefits

✅ **Centralized control** - Enable/disable via CMake, not code changes
✅ **Zero runtime cost** - Disabled logs compile out completely
✅ **Organized categories** - Debug specific subsystems independently
✅ **Version control friendly** - No more toggling counters in source
✅ **Build system integration** - Different configs for dev/release

## Future Categories

Easy to add new categories:

1. Add option to `CMakeLists.txt`:
   ```cmake
   option(RD_DEBUG_NEW_CATEGORY "Enable new debug logging" OFF)
   ```

2. Add to compile definitions (2 places):
   ```cmake
   RD_DEBUG_NEW_CATEGORY=$<BOOL:${RD_DEBUG_NEW_CATEGORY}>
   ```

3. Add flag to `DebugLog.h`:
   ```cpp
   #ifndef RD_DEBUG_NEW_CATEGORY
   #define RD_DEBUG_NEW_CATEGORY 0
   #endif
   ```

4. Optionally add convenience macro:
   ```cpp
   #define LOG_NEW_CATEGORY(max, code) \
       LOG_ONCE(RD_DEBUG_NEW_CATEGORY, max, code)
   ```
