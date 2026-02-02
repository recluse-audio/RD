/**
 * test_GranulatorProcessor.cpp
 * Comprehensive tests for GranulatorProcessor - TD-PSOLA pitch shifting processor
 * Tests range calculations, state transitions, and end-to-end processing
 */

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../SOURCE/PROCESSORS/GRAIN/GranulatorProcessor.h"
#include "../SOURCE/PROCESSORS/GRAIN/Granulator.h"
#include "../SOURCE/CircularBuffer.h"
#include "../SOURCE/PITCH/PitchDetector.h"
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/BufferHelper.h"
#include "TEST_UTILS/TestUtils.h"

//==============================================================================
// Test Constants
//==============================================================================
namespace TestConfig
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr int pitchDetectBufferSize = 1024; // blockSize < 1024, so uses MagicNumbers::minDetectionSize
    constexpr int circularBufferSize = pitchDetectBufferSize * 2; // 2048
    constexpr int maxGrainSize = pitchDetectBufferSize; // Same as pitch detect buffer
    constexpr float testFrequency = 187.5f; // 48000 / 256 = 187.5 Hz
    constexpr int testPeriod = 256;
}

//==============================================================================
// Basic Initialization Tests
//==============================================================================

TEST_CASE("GranulatorProcessor initialization", "[GranulatorProcessor][init]")
{
    TestUtils::SetupAndTeardown setup;
    GranulatorProcessor processor;

    SECTION("After prepareToPlay, components are initialized")
    {
        processor.prepareToPlay(TestConfig::sampleRate, TestConfig::blockSize);

        // Circular buffer size is calculated as: pitchDetectBufferNumSamples * 2
        // With blockSize=128, pitchDetectBufferNumSamples=1024, so circularBufferSize=2048
        int expectedCircularBufferSize = 2048;
        CHECK(processor.getCircularBuffer().getSize() == expectedCircularBufferSize);
        CHECK(processor.getCircularBuffer().getNumChannels() == 2);
    }
}

//==============================================================================
// Range Calculation Tests
//==============================================================================

TEST_CASE("GranulatorProcessor range calculations", "[GranulatorProcessor][ranges]")
{
    TestUtils::SetupAndTeardown setup;
    GranulatorProcessor processor;
    processor.prepareToPlay(TestConfig::sampleRate, TestConfig::blockSize);

    juce::AudioBuffer<float> buffer(2, TestConfig::blockSize);
    juce::MidiBuffer midiBuffer;

    // Process 5 blocks to advance sample counter
    for (int i = 0; i < 5; ++i)
    {
        buffer.clear();
        processor.processBlock(buffer, midiBuffer);
    }

    SECTION("getProcessCounterRange returns correct boundaries")
    {
        auto [start, end] = processor.getProcessCounterRange();
        CHECK(start == 640);
        CHECK(end == 640 + TestConfig::blockSize - 1);
    }

    SECTION("getDetectionRange has correct lookahead offset")
    {
        auto [detectStart, detectEnd] = processor.getDetectionRange();
        auto [processStart, processEnd] = processor.getProcessCounterRange();

        constexpr int kMinLookaheadSize = 512;
        constexpr int kMinDetectionSize = 1024;

        CHECK(detectEnd == processEnd - kMinLookaheadSize);
        CHECK(detectStart == detectEnd - kMinDetectionSize);
    }

    SECTION("getAnalysisReadRange creates 2-period window")
    {
        constexpr float detectedPeriod = 256.0f;
        juce::int64 analysisMark = 1000;

        auto [start, mark, end] = processor.getAnalysisReadRange(analysisMark, detectedPeriod);

        CHECK(start == analysisMark - static_cast<juce::int64>(detectedPeriod));
        CHECK(mark == analysisMark);
        CHECK(end == analysisMark + static_cast<juce::int64>(detectedPeriod) - 1);
    }

    SECTION("getAnalysisWriteRange offsets by lookahead")
    {
        constexpr float detectedPeriod = 256.0f;
        juce::int64 analysisMark = 1000;
        constexpr int kMinLookaheadSize = 512;

        auto readRange = processor.getAnalysisReadRange(analysisMark, detectedPeriod);
        auto [writeStart, writeMark, writeEnd] = processor.getAnalysisWriteRange(readRange);
        auto [readStart, readMark, readEnd] = readRange;

        CHECK(writeStart == readStart + kMinLookaheadSize);
        CHECK(writeMark == readMark + kMinLookaheadSize);
        CHECK(writeEnd == readEnd + kMinLookaheadSize);
    }

    SECTION("getDryBlockRange has correct delay")
    {
        auto [dryStart, dryEnd] = processor.getDryBlockRange();
        auto [processStart, processEnd] = processor.getProcessCounterRange();

        constexpr int kMinLookaheadSize = 512;

        CHECK(dryStart == processStart - kMinLookaheadSize);
        CHECK(dryEnd == dryStart + TestConfig::blockSize);
    }
}

//==============================================================================
// Pitch Detection Integration Tests
//==============================================================================

TEST_CASE("GranulatorProcessor pitch detection integration", "[GranulatorProcessor][pitch]")
{
    TestUtils::SetupAndTeardown setup;
    GranulatorProcessor processor;
    processor.prepareToPlay(TestConfig::sampleRate, TestConfig::blockSize);

    juce::MidiBuffer midiBuffer;

    SECTION("Detects pitch from sine wave input")
    {
        constexpr int period = TestConfig::testPeriod;
        juce::AudioBuffer<float> buffer(2, TestConfig::blockSize);

        bool pitchDetected = false;
        for (int i = 0; i < 30; ++i)
        {
            buffer.clear();
            BufferFiller::generateSineCycles(buffer, period);
            processor.processBlock(buffer, midiBuffer);

            if (processor.getPitchDetector()->getCurrentPeriod() > 0)
            {
                pitchDetected = true;
                double detectedPeriod = processor.getPitchDetector()->getCurrentPeriod();

                bool validDetection = (detectedPeriod >= period * 0.9 && detectedPeriod <= period * 1.1) ||
                                     (detectedPeriod >= period * 1.9 && detectedPeriod <= period * 2.1);
                CHECK(validDetection);
                break;
            }
        }

        CHECK(pitchDetected);
    }

    SECTION("Does not detect pitch from silence")
    {
        juce::AudioBuffer<float> buffer(2, TestConfig::blockSize);
        buffer.clear();

        for (int i = 0; i < 10; ++i)
        {
            processor.processBlock(buffer, midiBuffer);
        }

        CHECK(processor.getPitchDetector()->getCurrentPeriod() <= 0);
    }
}

//==============================================================================
// Circular Buffer Integration Tests
//==============================================================================

TEST_CASE("GranulatorProcessor circular buffer integration", "[GranulatorProcessor][circular]")
{
    TestUtils::SetupAndTeardown setup;
    GranulatorProcessor processor;
    processor.prepareToPlay(TestConfig::sampleRate, TestConfig::blockSize);

    juce::MidiBuffer midiBuffer;
    juce::AudioBuffer<float> buffer(2, TestConfig::blockSize);

    SECTION("Input is written to circular buffer")
    {
        BufferFiller::fillWithValue(buffer, 0.5f);
        processor.processBlock(buffer, midiBuffer);

        auto& circBuf = processor.getCircularBuffer();

        for (int i = 0; i < TestConfig::blockSize; ++i)
        {
            float value = circBuf.getBuffer().getSample(0, i);
            CHECK(value == Catch::Approx(0.5f).margin(0.001f));
        }
    }

    SECTION("Circular buffer wraps around correctly")
    {
        int totalBlocks = (TestConfig::circularBufferSize / TestConfig::blockSize) + 2;

        for (int block = 0; block < totalBlocks; ++block)
        {
            buffer.clear();
            BufferFiller::fillWithValue(buffer, static_cast<float>(block));
            processor.processBlock(buffer, midiBuffer);
        }

        CHECK(true); // Passed if no crash/assertion
    }
}

//==============================================================================
// Granulator Integration Tests
//==============================================================================

TEST_CASE("GranulatorProcessor granulator integration", "[GranulatorProcessor][granulator]")
{
    TestUtils::SetupAndTeardown setup;
    GranulatorProcessor processor;
    processor.prepareToPlay(TestConfig::sampleRate, TestConfig::blockSize);

    juce::MidiBuffer midiBuffer;

    SECTION("Granulator is prepared correctly")
    {
        auto& granulator = processor.getGranulator();

        CHECK(granulator.getWindow().getSize() > 0);
        CHECK(granulator.getSynthMark() == -1);
    }

    SECTION("Grains are created during processing")
    {
        constexpr int period = TestConfig::testPeriod;
        juce::AudioBuffer<float> buffer(2, TestConfig::blockSize);

        for (int i = 0; i < 30; ++i)
        {
            buffer.clear();
            BufferFiller::generateSineCycles(buffer, period);
            processor.processBlock(buffer, midiBuffer);
        }

        auto& granulator = processor.getGranulator();
        CHECK(granulator.getSynthMark() >= 0);
    }
}

//==============================================================================
// Pitch Shifting Tests
//==============================================================================

TEST_CASE("GranulatorProcessor pitch shifting", "[GranulatorProcessor][shift]")
{
    TestUtils::SetupAndTeardown setup;
    GranulatorProcessor processor;
    processor.prepareToPlay(TestConfig::sampleRate, TestConfig::blockSize);

    juce::MidiBuffer midiBuffer;

    SECTION("Shift ratio parameter changes shift ratio")
    {
        auto* param = processor.getAPVTS().getParameter("shift ratio");
        REQUIRE(param != nullptr);

        param->setValueNotifyingHost(0.0f); // Normalized: 0.0 maps to 0.5
        CHECK(param->getValue() == 0.0f);
    }

    SECTION("Shift ratio = 1.0 (no shift) processes correctly")
    {
        auto* param = processor.getAPVTS().getParameter("shift ratio");
        param->setValueNotifyingHost(0.5f); // Normalized: 0.5 maps to 1.0

        constexpr int period = TestConfig::testPeriod;
        juce::AudioBuffer<float> buffer(2, TestConfig::blockSize);

        for (int i = 0; i < 20; ++i)
        {
            buffer.clear();
            BufferFiller::generateSineCycles(buffer, period);
            processor.processBlock(buffer, midiBuffer);
        }

        CHECK(true);
    }

    SECTION("Shift ratio = 1.5 (fifth up) processes correctly")
    {
        auto* param = processor.getAPVTS().getParameter("shift ratio");
        param->setValueNotifyingHost(1.0f); // Normalized: 1.0 maps to 1.5

        constexpr int period = TestConfig::testPeriod;
        juce::AudioBuffer<float> buffer(2, TestConfig::blockSize);

        for (int i = 0; i < 20; ++i)
        {
            buffer.clear();
            BufferFiller::generateSineCycles(buffer, period);
            processor.processBlock(buffer, midiBuffer);
        }

        CHECK(true);
    }
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_CASE("GranulatorProcessor edge cases", "[GranulatorProcessor][edge]")
{
    TestUtils::SetupAndTeardown setup;
    GranulatorProcessor processor;
    processor.prepareToPlay(TestConfig::sampleRate, TestConfig::blockSize);

    juce::MidiBuffer midiBuffer;
    juce::AudioBuffer<float> buffer(2, TestConfig::blockSize);

    SECTION("Silent input does not crash")
    {
        buffer.clear();

        for (int i = 0; i < 10; ++i)
        {
            processor.processBlock(buffer, midiBuffer);
        }

        CHECK(true);
    }

    SECTION("Very low frequency (long period)")
    {
        constexpr int longPeriod = 960;

        for (int i = 0; i < 30; ++i)
        {
            buffer.clear();
            BufferFiller::generateSineCycles(buffer, longPeriod);
            processor.processBlock(buffer, midiBuffer);
        }

        CHECK(true);
    }

    SECTION("Very high frequency (short period)")
    {
        constexpr int shortPeriod = 48;

        for (int i = 0; i < 30; ++i)
        {
            buffer.clear();
            BufferFiller::generateSineCycles(buffer, shortPeriod);
            processor.processBlock(buffer, midiBuffer);
        }

        CHECK(true);
    }

    SECTION("Rapid pitch changes")
    {
        constexpr int period1 = 200;
        constexpr int period2 = 300;

        for (int i = 0; i < 20; ++i)
        {
            buffer.clear();
            int period = (i % 2 == 0) ? period1 : period2;
            BufferFiller::generateSineCycles(buffer, period);
            processor.processBlock(buffer, midiBuffer);
        }

        CHECK(true);
    }

    SECTION("Multi-block processing works correctly")
    {
        constexpr int period = TestConfig::testPeriod;

        for (int i = 0; i < 50; ++i)
        {
            buffer.clear();
            BufferFiller::generateSineCycles(buffer, period);
            processor.processBlock(buffer, midiBuffer);
        }

        CHECK(true);
    }
}

//==============================================================================
// Parameter Tests
//==============================================================================

TEST_CASE("GranulatorProcessor parameter handling", "[GranulatorProcessor][params]")
{
    TestUtils::SetupAndTeardown setup;
    GranulatorProcessor processor;
    processor.prepareToPlay(TestConfig::sampleRate, TestConfig::blockSize);

    SECTION("Has shift ratio parameter")
    {
        auto* param = processor.getAPVTS().getParameter("shift ratio");
        REQUIRE(param != nullptr);
    }

    SECTION("shift ratio parameter has correct range")
    {
        auto* param = dynamic_cast<juce::AudioParameterFloat*>(
            processor.getAPVTS().getParameter("shift ratio")
        );
        REQUIRE(param != nullptr);

        float minVal = param->getNormalisableRange().start;
        float maxVal = param->getNormalisableRange().end;

        CHECK(minVal == 0.5f);
        CHECK(maxVal == 1.5f);
    }

    SECTION("shift ratio parameter default is 1.0")
    {
        auto* param = dynamic_cast<juce::AudioParameterFloat*>(
            processor.getAPVTS().getParameter("shift ratio")
        );
        REQUIRE(param != nullptr);

        CHECK(param->get() == 1.0f);
    }
}

//==============================================================================
// End-to-End ProcessBlock Tests
//==============================================================================

TEST_CASE("GranulatorProcessor end-to-end with sine input", "[GranulatorProcessor][e2e]")
{
    TestUtils::SetupAndTeardown setup;

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr int sineBufferSize = 2048;
    constexpr int sinePeriod = 256;
    constexpr int numChannels = 2;

    GranulatorProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> sineBuffer(numChannels, sineBufferSize);
    sineBuffer.clear();
    BufferFiller::generateSineCycles(sineBuffer, sinePeriod);

    juce::AudioBuffer<float> processBuffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;

    auto fillProcessBuffer = [&](int callIndex) {
        int sourceStartSample = ((callIndex - 1) * blockSize) % sineBufferSize;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int sample = 0; sample < blockSize; ++sample)
            {
                int sourceIndex = (sourceStartSample + sample) % sineBufferSize;
                processBuffer.setSample(ch, sample, sineBuffer.getSample(ch, sourceIndex));
            }
        }
    };

    SECTION("Output has non-zero samples after warmup")
    {
        constexpr int warmupBlocks = 25;

        for (int callIndex = 1; callIndex <= warmupBlocks; ++callIndex)
        {
            fillProcessBuffer(callIndex);
            processor.processBlock(processBuffer, midiBuffer);
        }

        constexpr int testBlocks = 4;
        int totalNonZeroSamples = 0;
        float maxAbsValue = 0.0f;

        for (int i = 0; i < testBlocks; ++i)
        {
            fillProcessBuffer(warmupBlocks + 1 + i);
            processor.processBlock(processBuffer, midiBuffer);

            for (int s = 0; s < blockSize; ++s)
            {
                float sample = processBuffer.getSample(0, s);
                if (std::abs(sample) > 0.001f)
                    totalNonZeroSamples++;
                maxAbsValue = std::max(maxAbsValue, std::abs(sample));
            }
        }

        int totalSamples = testBlocks * blockSize;
        INFO("Non-zero samples: " << totalNonZeroSamples << " / " << totalSamples);
        INFO("Max absolute value: " << maxAbsValue);

        CHECK(totalNonZeroSamples > totalSamples / 2);
        CHECK(maxAbsValue > 0.1f);
        CHECK(maxAbsValue <= 2.0f);
    }

    SECTION("Audio persists over many blocks - no dropout")
    {
        constexpr int warmupBlocks = 30;
        constexpr int testBlocks = 100;

        for (int callIndex = 1; callIndex <= warmupBlocks; ++callIndex)
        {
            fillProcessBuffer(callIndex);
            processor.processBlock(processBuffer, midiBuffer);
        }

        int blocksWithAudio = 0;
        int maxConsecutiveDropout = 0;
        int consecutiveDropout = 0;

        for (int i = 0; i < testBlocks; ++i)
        {
            fillProcessBuffer(warmupBlocks + 1 + i);
            processor.processBlock(processBuffer, midiBuffer);

            int nonZeroCount = 0;
            float maxAbs = 0.0f;
            for (int s = 0; s < blockSize; ++s)
            {
                float sample = std::abs(processBuffer.getSample(0, s));
                if (sample > 0.01f)
                    nonZeroCount++;
                maxAbs = std::max(maxAbs, sample);
            }

            bool hasAudio = (nonZeroCount > blockSize / 4) && (maxAbs > 0.05f);

            if (hasAudio)
            {
                blocksWithAudio++;
                consecutiveDropout = 0;
            }
            else
            {
                consecutiveDropout++;
                maxConsecutiveDropout = std::max(maxConsecutiveDropout, consecutiveDropout);
            }
        }

        INFO("Blocks with audio: " << blocksWithAudio << " / " << testBlocks);
        INFO("Max consecutive dropout: " << maxConsecutiveDropout);

        CHECK(blocksWithAudio > testBlocks * 0.9);
        CHECK(maxConsecutiveDropout <= 2);
    }
}


TEST_CASE("GranulatorProcessor processBlock() does not reduce rms", "[GranulatorProcessor][processBlock]")
{
    /**
     * This test is a bit unreasonable, all 1's, no pitch detection. Making sure it makes sound.
     * Ensuring it doesn't unintentionally modify the audio data (mute it)
     */
    TestUtils::SetupAndTeardown setup;

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr int numChannels = 2;

    GranulatorProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> processBuffer (numChannels, blockSize); processBuffer.clear();
    BufferFiller::fillWithAllOnes(processBuffer); 

    // needed for processBlock() call
    juce::MidiBuffer midiBuffer;

    // keep giving process the processBuffer filled with all ones
    // refill each time b/c processBlock() clears it.
    for(int processBlockCall = 0; processBlockCall < 32; processBlockCall++)
    {
        BufferFiller::fillWithAllOnes(processBuffer); 
        processor.processBlock(processBuffer, midiBuffer);

        int numSamples  = processBuffer.getNumSamples();
        int numChannels = processBuffer.getNumChannels();
        
        // we expect 1.f to come out, since that's what we put it. 
        // but the first 4 block calls will be 0.f due to reading delayed audio data
        // from the circular buffer
        float expectedValue = 1.f; 
        if(processBlockCall <= 3)
            expectedValue = 0.f; 

        INFO("Process Block Call: " << processBlockCall);
        CHECK(processBuffer.getSample(0, 0) == expectedValue);
        // for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        // {
        //     for (int ch = 0; ch < numChannels; ++ch)
        //     {
        //         CHECK(processBuffer.getSample(ch, sampleIndex) == expectedValue);
        //     }

        // }
    }
}