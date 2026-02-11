/**
 * DebugLogger.h
 * Created by Ryan Devens
 *
 * Clean interface for debug logging across RD submodule.
 *
 * Usage:
 * - Compile-time guards (CMake flags) control whether calls exist at all
 * - Runtime flags control whether enabled calls actually output
 * - Perfect for testing: enable specific categories per-test without recompiling
 */

#pragma once
#include <iostream>
#include <vector>
#include <tuple>
#include <juce_core/juce_core.h>

class DebugLogger
{
public:
    //==========================================================================
    // Runtime Enable/Disable Controls
    //==========================================================================

    static void enablePitchDetection(bool enable = true);
    static void enableGrainCreation(bool enable = true);
    static void enableGrainProcessing(bool enable = true);
    static void enableSampleDetail(bool enable = true);
    static void enableOutputStats(bool enable = true);
    static void enableGainProcessing(bool enable = true);
    static void enableAll(bool enable = true);

    static bool isPitchDetectionEnabled();
    static bool isGrainCreationEnabled();
    static bool isGrainProcessingEnabled();
    static bool isSampleDetailEnabled();
    static bool isOutputStatsEnabled();
    static bool isGainProcessingEnabled();

    //==========================================================================
    // Pitch Detection Logging
    //==========================================================================

    static void logPitchDetection(int& blockCounter, int maxCount,
                                   float detectedPeriod, juce::int64 samplesProcessed,
                                   juce::int64 minSamplesForDetection);

    static void logPitchShift(int maxCount, float shiftRatio,
                              float detectedPeriod, float shiftedPeriod);

    //==========================================================================
    // Grain Creation Logging
    //==========================================================================

    static void logGrainCreation(int maxCount,
                                  const std::tuple<juce::int64, juce::int64, juce::int64>& analysisRange,
                                  const std::tuple<juce::int64, juce::int64, juce::int64>& synthRange,
                                  juce::int64 readStart, juce::int64 readEnd,
                                  const std::vector<float>& sampledRead,
                                  const std::vector<float>& sampledWindowed);

    static void logMultipleGrains(int grainsToCreate, juce::int64 synthMark,
                                  juce::int64 currentAnalysisWriteMark,
                                  juce::int64 nextAnalysisWriteMark,
                                  juce::int64 diff);

    //==========================================================================
    // Grain Processing Logging
    //==========================================================================

    static void logGrainProcessing(int maxCount, juce::int64 blockStart,
                                    juce::int64 blockEnd, int activeGrains);

    static void logGrainOverlap(juce::int64 synthStart, juce::int64 synthEnd);

    static void logGrainPast(juce::int64 synthStart, juce::int64 synthEnd);

    //==========================================================================
    // Sample Detail Logging
    //==========================================================================

    static void logSampleDetail(bool condition, juce::int64 sampleCount,
                                 int blockIndex, int grainBufferIndex, float grainValue);

    //==========================================================================
    // Output Stats Logging
    //==========================================================================

    static void logOutputStats(int maxCount, int samplesReplaced, int samplesLeftDry);

    //==========================================================================
    // Gain Processing Logging
    //==========================================================================

    static void logGainProcessing(int maxCount, float originalRMS, float newRMS, float gain);

private:
    //==========================================================================
    // Runtime State
    //==========================================================================

    static bool sPitchDetectionEnabled;
    static bool sGrainCreationEnabled;
    static bool sGrainProcessingEnabled;
    static bool sSampleDetailEnabled;
    static bool sOutputStatsEnabled;
    static bool sGainProcessingEnabled;
};
