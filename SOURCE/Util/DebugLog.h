/**
 * DebugLog.h
 * Created by Ryan Devens
 *
 * Compile-time debug logging configuration.
 * Enable specific debug categories via CMake options.
 *
 * CMake Usage:
 *   cmake -DRD_DEBUG_GRAIN_CREATION=ON ...
 *   cmake -DRD_DEBUG_ALL=ON ...
 */

#pragma once
#include <iostream>

// =============================================================================
// Debug Category Flags (set via CMake)
// =============================================================================

// Individual category flags
#ifndef RD_DEBUG_GRAIN_CREATION
#define RD_DEBUG_GRAIN_CREATION 0
#endif

#ifndef RD_DEBUG_GRAIN_PROCESSING
#define RD_DEBUG_GRAIN_PROCESSING 0
#endif

#ifndef RD_DEBUG_PITCH_DETECTION
#define RD_DEBUG_PITCH_DETECTION 0
#endif

#ifndef RD_DEBUG_SAMPLE_DETAIL
#define RD_DEBUG_SAMPLE_DETAIL 0
#endif

#ifndef RD_DEBUG_OUTPUT_STATS
#define RD_DEBUG_OUTPUT_STATS 0
#endif

#ifndef RD_DEBUG_GAIN_PROCESSING
#define RD_DEBUG_GAIN_PROCESSING 0
#endif

// Master debug flag (enables all)
#ifndef RD_DEBUG_ALL
#define RD_DEBUG_ALL 0
#endif

// =============================================================================
// Debug Logging Macros
// =============================================================================

/**
 * LOG_ONCE(category, max_count, code)
 * Executes code only for the first max_count times.
 * Tracks state with static counter.
 *
 * Example:
 *   LOG_ONCE(RD_DEBUG_GRAIN_CREATION, 3, {
 *       std::cout << "Grain #" << grain_id << std::endl;
 *   });
 */
#define LOG_ONCE(category, max_count, code) \
    do { \
        if ((category) || RD_DEBUG_ALL) { \
            static int log_counter = 0; \
            if (log_counter < (max_count)) { \
                code \
                log_counter++; \
            } \
        } \
    } while(0)

/**
 * LOG_COUNTED(category, max_count, counter_var, code)
 * Executes code only while counter_var < max_count.
 * Uses provided counter variable instead of static.
 *
 * Example:
 *   int block_num = 0;
 *   // ... in loop ...
 *   LOG_COUNTED(RD_DEBUG_PITCH_DETECTION, 25, block_num, {
 *       std::cout << "Block " << block_num << std::endl;
 *   });
 *   block_num++;
 */
#define LOG_COUNTED(category, max_count, counter_var, code) \
    do { \
        if (((category) || RD_DEBUG_ALL) && (counter_var) < (max_count)) { \
            code \
        } \
    } while(0)

/**
 * LOG_ALWAYS(category, code)
 * Executes code every time if category is enabled.
 *
 * Example:
 *   LOG_ALWAYS(RD_DEBUG_GRAIN_PROCESSING, {
 *       std::cout << "Processing grain" << std::endl;
 *   });
 */
#define LOG_ALWAYS(category, code) \
    do { \
        if ((category) || RD_DEBUG_ALL) { \
            code \
        } \
    } while(0)

/**
 * LOG_CONDITIONAL(category, condition, code)
 * Executes code when category is enabled AND condition is true.
 *
 * Example:
 *   LOG_CONDITIONAL(RD_DEBUG_SAMPLE_DETAIL, sample_index == 0, {
 *       std::cout << "First sample" << std::endl;
 *   });
 */
#define LOG_CONDITIONAL(category, condition, code) \
    do { \
        if (((category) || RD_DEBUG_ALL) && (condition)) { \
            code \
        } \
    } while(0)

// =============================================================================
// Convenience Macros for Specific Categories
// =============================================================================

#define LOG_GRAIN_CREATION(max_count, code) \
    LOG_ONCE(RD_DEBUG_GRAIN_CREATION, max_count, code)

#define LOG_GRAIN_PROCESSING(max_count, code) \
    LOG_ONCE(RD_DEBUG_GRAIN_PROCESSING, max_count, code)

#define LOG_PITCH_DETECTION(max_count, counter, code) \
    LOG_COUNTED(RD_DEBUG_PITCH_DETECTION, max_count, counter, code)

#define LOG_SAMPLE_DETAIL(condition, code) \
    LOG_CONDITIONAL(RD_DEBUG_SAMPLE_DETAIL, condition, code)

#define LOG_OUTPUT_STATS(max_count, code) \
    LOG_ONCE(RD_DEBUG_OUTPUT_STATS, max_count, code)

#define LOG_GAIN_PROCESSING(max_count, code) \
    LOG_ONCE(RD_DEBUG_GAIN_PROCESSING, max_count, code)
