/**
 * test_Granulator.cpp
 * Comprehensive tests for Granulator utility class
 * Tests TD-PSOLA grain-based time-stretching and pitch-shifting
 */

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../SOURCE/PROCESSORS/GRAIN/Granulator.h"
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/BufferHelper.h"
#include "../SOURCE/CircularBuffer.h"
#include "../SOURCE/Window.h"
#include "TEST_UTILS/TestUtils.h"

//=======================================================================================
// PREPARE TESTS
//=======================================================================================
TEST_CASE("Granulator prepare() initializes all grains as inactive", "[Granulator]")
{
    Granulator granulator;
    granulator.prepare(48000.0, 512, 2048);

    auto& grains = granulator.getGrains();
    for (int i = 0; i < kNumGrains; ++i)
    {
        REQUIRE(grains[i].isActive == false);
    }

    CHECK(granulator.getSynthMark() == -1);
    CHECK(granulator.getWindow().getSize() == 48000);
    CHECK(granulator.getWindow().getReadPos() == 0);
    CHECK(granulator.getWindow().getPeriod() == 2048);
}

//=======================================================================================
// PROCESS TRACKING RANGE TESTS
//=======================================================================================
TEST_CASE("Granulator processTracking() creates grain with correct ranges", "[Granulator][processTracking]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr int circularBufferSize = 2048;
    constexpr float detectedPeriod = 256.0f;
    constexpr float shiftedPeriod = 256.0f;

    Granulator granulator;
    int maxGrainSize = static_cast<int>(detectedPeriod * 2);
    granulator.prepare(sampleRate, blockSize, maxGrainSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(2, circularBufferSize);
    juce::AudioBuffer<float> sineBuffer(2, circularBufferSize);
    BufferFiller::generateSineCycles(sineBuffer, static_cast<int>(detectedPeriod));
    circularBuffer.pushBuffer(sineBuffer);

    juce::AudioBuffer<float> processBuffer(2, blockSize);
    processBuffer.clear();

    std::tuple<juce::int64, juce::int64, juce::int64> analysisReadRange = {744, 1000, 1255};
    std::tuple<juce::int64, juce::int64, juce::int64> analysisWriteRange = {1536, 1792, 2047};
    std::tuple<juce::int64, juce::int64> processCounterRange = {1536, 1663};

    granulator.processTracking(processBuffer, circularBuffer,
                              analysisReadRange, analysisWriteRange,
                              processCounterRange,
                              detectedPeriod, shiftedPeriod);

    SECTION("One grain is created and active")
    {
        auto& grains = granulator.getGrains();
        int activeCount = 0;
        for (int i = 0; i < kNumGrains; ++i)
        {
            if (grains[i].isActive)
                activeCount++;
        }
        CHECK(activeCount == 1);
    }

    SECTION("Grain has correct analysis range (744, 1000, 1255)")
    {
        auto& grains = granulator.getGrains();
        for (int i = 0; i < kNumGrains; ++i)
        {
            if (grains[i].isActive)
            {
                auto [start, mark, end] = grains[i].mAnalysisRange;
                CHECK(start == 744);
                CHECK(mark == 1000);
                CHECK(end == 1255);
                break;
            }
        }
    }

    SECTION("Grain has correct synth range (1536, 1792, 2047)")
    {
        auto& grains = granulator.getGrains();
        for (int i = 0; i < kNumGrains; ++i)
        {
            if (grains[i].isActive)
            {
                auto [start, mark, end] = grains[i].mSynthRange;
                CHECK(start == 1536);
                CHECK(mark == 1792);
                CHECK(end == 2047);
                break;
            }
        }
    }

    SECTION("SynthMark is updated to next position (2048)")
    {
        CHECK(granulator.getSynthMark() == 2048);
    }
}

//=======================================================================================
// MAKE GRAIN TESTS
//=======================================================================================
TEST_CASE("Granulator makeGrain() copies correct samples with no windowing", "[Granulator][makeGrain]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr int circularBufferSize = 512;
    constexpr float detectedPeriod = 100.0f;
    constexpr int grainSize = static_cast<int>(detectedPeriod * 2);

    Granulator granulator;
    granulator.prepare(sampleRate, blockSize, grainSize);
    granulator.getWindow().setSizeShapePeriod(static_cast<int>(sampleRate), Window::Shape::kNone, grainSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(2, circularBufferSize);
    juce::AudioBuffer<float> incrementalBuffer(2, circularBufferSize);
    BufferFiller::fillIncremental(incrementalBuffer);
    circularBuffer.pushBuffer(incrementalBuffer);

    std::tuple<juce::int64, juce::int64, juce::int64> analysisReadRange = {100, 200, 299};
    std::tuple<juce::int64, juce::int64, juce::int64> synthRange = {1000, 1100, 1199};

    granulator.makeGrain(circularBuffer, analysisReadRange, synthRange, detectedPeriod, detectedPeriod);

    auto& grains = granulator.getGrains();
    REQUIRE(grains[0].isActive == true);

    const auto& grainBuffer = grains[0].getBuffer();
    for (int i = 0; i < grainSize; ++i)
    {
        float expectedValue = static_cast<float>(100 + i);
        float actualCh0 = grainBuffer.getSample(0, i);
        float actualCh1 = grainBuffer.getSample(1, i);

        CHECK(actualCh0 == expectedValue);
        CHECK(actualCh1 == expectedValue);
    }
}

TEST_CASE("Granulator makeGrain() applies Hanning window correctly", "[Granulator][makeGrain]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr int circularBufferSize = 512;
    constexpr float detectedPeriod = 100.0f;
    constexpr int grainSize = static_cast<int>(detectedPeriod * 2);

    Granulator granulator;
    granulator.prepare(sampleRate, blockSize, grainSize);
    REQUIRE(granulator.getWindow().getShape() == Window::Shape::kHanning);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(2, circularBufferSize);
    juce::AudioBuffer<float> onesBuffer(2, circularBufferSize);
    BufferFiller::fillWithAllOnes(onesBuffer);
    circularBuffer.pushBuffer(onesBuffer);

    std::tuple<juce::int64, juce::int64, juce::int64> analysisReadRange = {0, 100, 199};
    std::tuple<juce::int64, juce::int64, juce::int64> synthRange = {1000, 1100, 1199};

    granulator.makeGrain(circularBuffer, analysisReadRange, synthRange, detectedPeriod, detectedPeriod);

    auto& grains = granulator.getGrains();
    REQUIRE(grains[0].isActive == true);

    const auto& grainBuffer = grains[0].getBuffer();
    for (int i = 0; i < grainSize; ++i)
    {
        float expectedValue = granulator.getWindow().getValueAtIndexInPeriod(i);
        CHECK(grainBuffer.getSample(0, i) == Catch::Approx(expectedValue).margin(0.001f));
        CHECK(grainBuffer.getSample(1, i) == Catch::Approx(expectedValue).margin(0.001f));
    }
}

//=======================================================================================
// MULTIPLE PROCESS TRACKING CALLS - OVERLAPPING GRAINS
//=======================================================================================
TEST_CASE("Granulator processTracking() manages multiple overlapping grains", "[Granulator][processTracking]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr int circularBufferSize = 4096;
    constexpr float detectedPeriod = 256.0f;
    constexpr float shiftedPeriod = 256.0f;

    Granulator granulator;
    int maxGrainSize = static_cast<int>(detectedPeriod * 2);
    granulator.prepare(sampleRate, blockSize, maxGrainSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(2, circularBufferSize);
    juce::AudioBuffer<float> sineBuffer(2, circularBufferSize);
    BufferFiller::generateSineCycles(sineBuffer, static_cast<int>(detectedPeriod));
    circularBuffer.pushBuffer(sineBuffer);

    juce::AudioBuffer<float> processBuffer(2, blockSize);

    auto countActiveGrains = [&granulator]() {
        int count = 0;
        for (const auto& grain : granulator.getGrains())
        {
            if (grain.isActive)
                count++;
        }
        return count;
    };

    SECTION("Call processTracking() 2x: Second grain created, two grains active")
    {
        processBuffer.clear();
        granulator.processTracking(processBuffer, circularBuffer,
                                  {744, 1000, 1255}, {1536, 1792, 2047},
                                  {1536, 1663},
                                  detectedPeriod, shiftedPeriod);
        CHECK(countActiveGrains() == 1);
        CHECK(granulator.getSynthMark() == 2048);

        processBuffer.clear();
        granulator.processTracking(processBuffer, circularBuffer,
                                  {1000, 1256, 1511}, {1792, 2048, 2303},
                                  {1664, 1791},
                                  detectedPeriod, shiftedPeriod);

        CHECK(countActiveGrains() == 2);
        CHECK(granulator.getSynthMark() == 2304);
    }

    SECTION("Grain deactivated when synthEnd <= blockEnd")
    {
        processBuffer.clear();
        granulator.processTracking(processBuffer, circularBuffer,
                                  {744, 1000, 1255}, {1536, 1792, 2047},
                                  {1536, 1663},
                                  detectedPeriod, shiftedPeriod);
        CHECK(countActiveGrains() == 1);

        processBuffer.clear();
        granulator.processTracking(processBuffer, circularBuffer,
                                  {1000, 1256, 1511}, {1792, 2048, 2303},
                                  {1664, 1791},
                                  detectedPeriod, shiftedPeriod);
        CHECK(countActiveGrains() == 2);

        processBuffer.clear();
        granulator.processTracking(processBuffer, circularBuffer,
                                  {1256, 1512, 1767}, {2048, 2304, 2559},
                                  {1792, 1919},
                                  detectedPeriod, shiftedPeriod);
        CHECK(countActiveGrains() == 3);

        processBuffer.clear();
        granulator.processTracking(processBuffer, circularBuffer,
                                  {1512, 1768, 2023}, {2304, 2560, 2815},
                                  {1920, 2047},
                                  detectedPeriod, shiftedPeriod);

        CHECK(countActiveGrains() == 3);
        CHECK(granulator.getSynthMark() == 2816);
    }
}

//=======================================================================================
// PITCH SHIFT UP - SHORTER SHIFTED PERIOD
//=======================================================================================
TEST_CASE("Granulator processTracking() with pitch shift up creates multiple grains", "[Granulator][processTracking][pitchShift]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr int circularBufferSize = 4096;
    constexpr float detectedPeriod = 256.0f;
    constexpr float shiftedPeriod = 192.0f; // Pitch shift UP

    Granulator granulator;
    int maxGrainSize = static_cast<int>(detectedPeriod * 2);
    granulator.prepare(sampleRate, blockSize, maxGrainSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(2, circularBufferSize);
    juce::AudioBuffer<float> sineBuffer(2, circularBufferSize);
    BufferFiller::generateSineCycles(sineBuffer, static_cast<int>(detectedPeriod));
    circularBuffer.pushBuffer(sineBuffer);

    juce::AudioBuffer<float> processBuffer(2, blockSize);

    auto countActiveGrains = [&granulator]() {
        int count = 0;
        for (const auto& grain : granulator.getGrains())
        {
            if (grain.isActive)
                count++;
        }
        return count;
    };

    SECTION("Single call creates 2 grains with same analysisRange")
    {
        processBuffer.clear();
        granulator.processTracking(processBuffer, circularBuffer,
                                  {744, 1000, 1255}, {1536, 1792, 2047},
                                  {1536, 1663},
                                  detectedPeriod, shiftedPeriod);

        CHECK(countActiveGrains() == 2);
        CHECK(granulator.getSynthMark() == 2176);

        auto& grains = granulator.getGrains();
        bool foundGrain1 = false;
        bool foundGrain2 = false;

        for (const auto& grain : grains)
        {
            if (!grain.isActive) continue;

            auto [aStart, aMark, aEnd] = grain.mAnalysisRange;
            CHECK(aStart == 744);
            CHECK(aMark == 1000);
            CHECK(aEnd == 1255);

            auto [sStart, sMark, sEnd] = grain.mSynthRange;
            if (sMark == 1792)
            {
                foundGrain1 = true;
                CHECK(sStart == 1536);
                CHECK(sEnd == 2047);
            }
            else if (sMark == 1984)
            {
                foundGrain2 = true;
                CHECK(sStart == 1728);
                CHECK(sEnd == 2239);
            }
        }

        CHECK(foundGrain1);
        CHECK(foundGrain2);
    }
}

//=======================================================================================
// PROCESS ACTIVE GRAINS TESTS
//=======================================================================================
TEST_CASE("Granulator processActiveGrains() with no active grains leaves buffer unchanged", "[Granulator][processActiveGrains]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr int maxGrainSize = 512;
    constexpr int circularBufferSize = 2048;

    Granulator granulator;
    granulator.prepare(sampleRate, blockSize, maxGrainSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(2, circularBufferSize);
    juce::AudioBuffer<float> silenceBuffer(2, circularBufferSize);
    silenceBuffer.clear();
    circularBuffer.pushBuffer(silenceBuffer);

    auto& grains = granulator.getGrains();
    for (int i = 0; i < kNumGrains; ++i)
    {
        REQUIRE(grains[i].isActive == false);
    }

    juce::AudioBuffer<float> processBuffer(2, blockSize);
    processBuffer.clear();

    std::tuple<juce::int64, juce::int64> processCounterRange = {0, blockSize - 1};
    granulator.processActiveGrains(processBuffer, circularBuffer, processCounterRange);

    // With no active grains, should output dry audio from circular buffer (which is silence)
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < blockSize; ++i)
        {
            CHECK(processBuffer.getSample(ch, i) == 0.0f);
        }
    }
}

TEST_CASE("Granulator processActiveGrains() with overlapping grains", "[Granulator][processActiveGrains]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr int circularBufferSize = 2048;
    constexpr float detectedPeriod = 256.0f;
    constexpr int grainSize = static_cast<int>(detectedPeriod * 2);

    Granulator granulator;
    granulator.prepare(sampleRate, blockSize, grainSize);
    granulator.getWindow().setSizeShapePeriod(static_cast<int>(sampleRate), Window::Shape::kNone, grainSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(2, circularBufferSize);
    juce::AudioBuffer<float> onesBuffer(2, circularBufferSize);
    BufferFiller::fillWithAllOnes(onesBuffer);
    circularBuffer.pushBuffer(onesBuffer);

    // Create two overlapping grains
    granulator.makeGrain(circularBuffer,
                        {0, 256, 511},
                        {100, 356, 611},
                        detectedPeriod,
                        detectedPeriod);

    granulator.makeGrain(circularBuffer,
                        {0, 256, 511},
                        {300, 556, 811},
                        detectedPeriod,
                        detectedPeriod);

    auto& grains = granulator.getGrains();
    int activeCount = 0;
    std::stringstream grainInfo;
    for (int i = 0; i < kNumGrains; ++i)
    {
        if (grains[i].isActive)
        {
            activeCount++;
            auto [sStart, sMark, sEnd] = grains[i].mSynthRange;
            grainInfo << "Grain " << i << ": (" << sStart << ", " << sMark << ", " << sEnd << ") ";
        }
    }
    INFO("Active grains after makeGrain calls: " << activeCount << " - " << grainInfo.str());
    REQUIRE(activeCount == 2);

    juce::AudioBuffer<float> processBuffer(2, blockSize);
    processBuffer.clear();

    std::tuple<juce::int64, juce::int64> processCounterRange = {400, 527};
    granulator.processActiveGrains(processBuffer, circularBuffer, processCounterRange);

    // Check that audio was output
    float minVal = 999.0f;
    float maxVal = -999.0f;
    int nonZeroCount = 0;

    for (int i = 0; i < blockSize; ++i)
    {
        float sample = processBuffer.getSample(0, i);
        if (sample != 0.0f)
        {
            nonZeroCount++;
            minVal = std::min(minVal, sample);
            maxVal = std::max(maxVal, sample);
        }
    }

    INFO("Non-zero samples: " << nonZeroCount);
    INFO("Min value: " << minVal << ", Max value: " << maxVal);

    // Both grains should produce output in this range
    CHECK(nonZeroCount == blockSize);
    // With no windowing and all-ones input, output should be at least 1.0
    CHECK(minVal >= 0.99f);
    CHECK(maxVal <= 2.01f); // Could be up to 2.0 if both grains contribute
}



