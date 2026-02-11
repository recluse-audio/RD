/**
 * DebugLogger.cpp
 * Created by Ryan Devens
 */

#include "DebugLogger.h"

//==============================================================================
// Static Member Initialization
//==============================================================================

bool DebugLogger::sPitchDetectionEnabled = true;
bool DebugLogger::sGrainCreationEnabled = true;
bool DebugLogger::sGrainProcessingEnabled = true;
bool DebugLogger::sSampleDetailEnabled = true;
bool DebugLogger::sOutputStatsEnabled = true;
bool DebugLogger::sGainProcessingEnabled = true;

//==============================================================================
// Runtime Enable/Disable Controls
//==============================================================================

void DebugLogger::enablePitchDetection(bool enable) { sPitchDetectionEnabled = enable; }
void DebugLogger::enableGrainCreation(bool enable) { sGrainCreationEnabled = enable; }
void DebugLogger::enableGrainProcessing(bool enable) { sGrainProcessingEnabled = enable; }
void DebugLogger::enableSampleDetail(bool enable) { sSampleDetailEnabled = enable; }
void DebugLogger::enableOutputStats(bool enable) { sOutputStatsEnabled = enable; }
void DebugLogger::enableGainProcessing(bool enable) { sGainProcessingEnabled = enable; }

void DebugLogger::enableAll(bool enable)
{
    sPitchDetectionEnabled = enable;
    sGrainCreationEnabled = enable;
    sGrainProcessingEnabled = enable;
    sSampleDetailEnabled = enable;
    sOutputStatsEnabled = enable;
    sGainProcessingEnabled = enable;
}

bool DebugLogger::isPitchDetectionEnabled() { return sPitchDetectionEnabled; }
bool DebugLogger::isGrainCreationEnabled() { return sGrainCreationEnabled; }
bool DebugLogger::isGrainProcessingEnabled() { return sGrainProcessingEnabled; }
bool DebugLogger::isSampleDetailEnabled() { return sSampleDetailEnabled; }
bool DebugLogger::isOutputStatsEnabled() { return sOutputStatsEnabled; }
bool DebugLogger::isGainProcessingEnabled() { return sGainProcessingEnabled; }

//==============================================================================
// Pitch Detection Logging
//==============================================================================

void DebugLogger::logPitchDetection(int& blockCounter, int maxCount,
                                     float detectedPeriod, juce::int64 samplesProcessed,
                                     juce::int64 minSamplesForDetection)
{
    if (!sPitchDetectionEnabled || blockCounter >= maxCount)
        return;

    std::cout << "Block #" << blockCounter << " - detected_period: " << detectedPeriod
              << ", mSamplesProcessed: " << samplesProcessed
              << ", minSamplesForDetection: " << minSamplesForDetection << std::endl;
}

void DebugLogger::logPitchShift(int maxCount, float shiftRatio,
                                 float detectedPeriod, float shiftedPeriod)
{
    if (!sPitchDetectionEnabled)
        return;

    static int callCount = 0;
    if (callCount >= maxCount)
        return;

    std::cout << "PITCH SHIFT DEBUG - mShiftRatio: " << shiftRatio
              << ", detectedPeriod: " << detectedPeriod
              << ", shiftedPeriod: " << shiftedPeriod << std::endl;
    callCount++;
}

//==============================================================================
// Grain Creation Logging
//==============================================================================

void DebugLogger::logGrainCreation(int maxCount,
                                    const std::tuple<juce::int64, juce::int64, juce::int64>& analysisRange,
                                    const std::tuple<juce::int64, juce::int64, juce::int64>& synthRange,
                                    juce::int64 readStart, juce::int64 readEnd,
                                    const std::vector<float>& sampledRead,
                                    const std::vector<float>& sampledWindowed)
{
    if (!sGrainCreationEnabled)
        return;

    static int grainCounter = 0;
    if (grainCounter >= maxCount)
        return;

    std::cout << "\n=== makeGrain #" << grainCounter << " ===" << std::endl;
    std::cout << "  analysisReadRange: [" << std::get<0>(analysisRange)
              << ", " << std::get<1>(analysisRange)
              << ", " << std::get<2>(analysisRange) << "]" << std::endl;
    std::cout << "  synthRange (output): [" << std::get<0>(synthRange)
              << ", " << std::get<1>(synthRange)
              << ", " << std::get<2>(synthRange) << "]" << std::endl;
    std::cout << "  Reading audio from circular buffer positions " << readStart
              << " to " << readEnd << std::endl;

    std::cout << "  Sampled input values from circular buffer: [";
    for (size_t i = 0; i < sampledRead.size(); ++i)
    {
        std::cout << sampledRead[i];
        if (i < sampledRead.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    std::cout << "  Sampled windowed grain values: [";
    for (size_t i = 0; i < sampledWindowed.size(); ++i)
    {
        std::cout << sampledWindowed[i];
        if (i < sampledWindowed.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    grainCounter++;
}

void DebugLogger::logMultipleGrains(int grainsToCreate, juce::int64 synthMark,
                                     juce::int64 currentAnalysisWriteMark,
                                     juce::int64 nextAnalysisWriteMark,
                                     juce::int64 diff)
{
    if (!sGrainCreationEnabled)
        return;

    static int callCount = 0;
    if (callCount >= 100)
        return;

    std::cout << "Multiple grains! count=" << grainsToCreate << " mSynthMark=" << synthMark
              << " currentAnalysisWriteMark=" << currentAnalysisWriteMark
              << " nextAnalysisWriteMark=" << nextAnalysisWriteMark
              << " diff=" << diff << std::endl;
    callCount++;
}

//==============================================================================
// Grain Processing Logging
//==============================================================================

void DebugLogger::logGrainProcessing(int maxCount, juce::int64 blockStart,
                                      juce::int64 blockEnd, int activeGrains)
{
    if (!sGrainProcessingEnabled)
        return;

    static int callCounter = 0;
    if (callCounter >= maxCount)
        return;

    std::cout << "\nprocessActiveGrains call #" << callCounter
              << ": blockRange=[" << blockStart << "," << blockEnd
              << "] activeGrains=" << activeGrains << std::endl;
    callCounter++;
}

void DebugLogger::logGrainOverlap(juce::int64 synthStart, juce::int64 synthEnd)
{
    if (!sGrainProcessingEnabled)
        return;

    static int callCount = 0;
    if (callCount >= 100)
        return;

    std::cout << "  Grain [" << synthStart << ", " << synthEnd
              << "] OVERLAPS, will contribute" << std::endl;
    callCount++;
}

void DebugLogger::logGrainPast(juce::int64 synthStart, juce::int64 synthEnd)
{
    if (!sGrainProcessingEnabled)
        return;

    // Commented out to reduce noise
    // std::cout << "  Grain [" << synthStart << ", " << synthEnd
    //           << "] is in PAST, deactivating" << std::endl;
    (void)synthStart; (void)synthEnd;
}

//==============================================================================
// Sample Detail Logging
//==============================================================================

void DebugLogger::logSampleDetail(bool condition, juce::int64 sampleCount,
                                   int blockIndex, int grainBufferIndex, float grainValue)
{
    if (!sSampleDetailEnabled || !condition)
        return;

    static int sampleLogCounter = 0;
    if (sampleLogCounter >= 3)
        return;

    std::cout << "    Sample " << sampleCount << ": blockIndex=" << blockIndex
              << ", grainBufferIndex=" << grainBufferIndex
              << ", grainValue=" << grainValue << std::endl;
    sampleLogCounter++;
}

//==============================================================================
// Output Stats Logging
//==============================================================================

void DebugLogger::logOutputStats(int maxCount, int samplesReplaced, int samplesLeftDry)
{
    if (!sOutputStatsEnabled)
        return;

    static int callCount = 0;
    if (callCount >= maxCount)
        return;

    std::cout << "  Output: " << samplesReplaced << " samples from grains, "
              << samplesLeftDry << " samples kept as dry" << std::endl;
    callCount++;
}

//==============================================================================
// Gain Processing Logging
//==============================================================================

void DebugLogger::logGainProcessing(int maxCount, float originalRMS, float newRMS, float gain)
{
    if (!sGainProcessingEnabled)
        return;

    static int callCount = 0;
    if (callCount >= maxCount)
        return;

    std::cout << "Amplifying detection buffer: RMS " << originalRMS
              << " -> " << newRMS
              << " (gain: " << gain << "x)" << std::endl;
    callCount++;
}
