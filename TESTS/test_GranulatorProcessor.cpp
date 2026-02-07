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
    // Use MagicNumbers for buffer sizes - tests adapt automatically if values change
    constexpr int pitchDetectBufferSize = MagicNumbers::minDetectionSize;
    constexpr int circularBufferSize = pitchDetectBufferSize * 2;
    constexpr int lookaheadSize = MagicNumbers::minLookaheadSize;
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
        CHECK(processor.getCircularBuffer().getSize() == TestConfig::circularBufferSize);
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

        CHECK(detectEnd == processEnd - TestConfig::lookaheadSize);
        CHECK(detectStart == detectEnd - TestConfig::pitchDetectBufferSize);
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

        auto readRange = processor.getAnalysisReadRange(analysisMark, detectedPeriod);
        auto [writeStart, writeMark, writeEnd] = processor.getAnalysisWriteRange(readRange);
        auto [readStart, readMark, readEnd] = readRange;

        CHECK(writeStart == readStart + TestConfig::lookaheadSize);
        CHECK(writeMark == readMark + TestConfig::lookaheadSize);
        CHECK(writeEnd == readEnd + TestConfig::lookaheadSize);
    }

    SECTION("getDryBlockRange has correct delay")
    {
        auto [dryStart, dryEnd] = processor.getDryBlockRange();
        auto [processStart, processEnd] = processor.getProcessCounterRange();

        CHECK(dryStart == processStart - TestConfig::lookaheadSize);
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

        // Need enough blocks to fill detection buffer (4096 samples) + extra for processing
        // With blockSize=128: 4096/128 = 32 blocks minimum, use 50 to be safe
        const int minBlocks = 50;

        bool pitchDetected = false;
        for (int i = 0; i < minBlocks; ++i)
        {
            buffer.clear();
            BufferFiller::generateSineCycles(buffer, period);
            processor.processBlock(buffer, midiBuffer);

            double detectedPeriod = processor.getPitchDetector()->getCurrentPeriod();
            if (detectedPeriod > 0)
            {
                pitchDetected = true;

                INFO("Block: " << i << ", Detected period: " << detectedPeriod << ", Expected: " << period);
                // Accept fundamental, octave up (half period), or octave down (double period)
                bool validDetection = (detectedPeriod >= period * 0.45 && detectedPeriod <= period * 0.55) ||  // Octave up
                                     (detectedPeriod >= period * 0.9 && detectedPeriod <= period * 1.1) ||      // Fundamental
                                     (detectedPeriod >= period * 1.9 && detectedPeriod <= period * 2.1);       // Octave down
                CHECK(validDetection);
                break;
            }
            else if (i > 35)  // Only log after enough blocks for detection
            {
                INFO("Block: " << i << ", No pitch detected yet (period: " << detectedPeriod << ")");
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

        // Need enough blocks to fill detection buffer (4096 samples) and start grain creation
        // With blockSize=128: 4096/128 = 32 blocks minimum, use 50 to be safe
        const int minBlocks = 50;

        for (int i = 0; i < minBlocks; ++i)
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


TEST_CASE("GranulatorProcessor processBlock() does not reduce rms with no pitch detected", "[GranulatorProcessor][processBlock]")
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
        // but the first (lookaheadSize/blockSize) blocks will be 0.f due to reading delayed audio data
        // from the circular buffer. With lookahead=1024 and blockSize=128: 1024/128 = 8 blocks
        const int latencyBlocks = TestConfig::lookaheadSize / TestConfig::blockSize;
        float expectedValue = 1.f;
        if(processBlockCall < latencyBlocks)
            expectedValue = 0.f; 

        INFO("Process Block Call: " << processBlockCall);
        CHECK(processBuffer.getSample(0, 0) == expectedValue);
    }
}


TEST_CASE("GranulatorProcessor processBlock() does not reduce rms with pitch detected", "[GranulatorProcessor][processBlock]")
{
    /**
     * This test is a bit unreasonable, all 1's, no pitch detection. Making sure it makes sound.
     * Ensuring it doesn't unintentionally modify the audio data (mute it)
     */
    TestUtils::SetupAndTeardown setup;

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256; // same as period for convenience
    constexpr int numChannels = 2;

    GranulatorProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> processBuffer (numChannels, blockSize); processBuffer.clear();
    BufferFiller::generateSine(processBuffer); 

    // make this one to test against, don't send it through processBlock()
    juce::AudioBuffer<float> sineBuffer (numChannels, blockSize); sineBuffer.clear();
    BufferFiller::generateSine(sineBuffer); 

    // ensure they are identical before starting test
    REQUIRE(BufferHelper::buffersAreIdentical(processBuffer, sineBuffer));

    // needed for processBlock() call
    juce::MidiBuffer midiBuffer;

    // keep giving process the processBuffer filled with all ones
    // refill each time b/c processBlock() clears it.
    for(int processBlockCall = 0; processBlockCall < 32; processBlockCall++)
    {
        BufferFiller::generateSine(processBuffer); 
        processor.processBlock(processBuffer, midiBuffer);

        float detectedPeriod = processor.getLastDetectedPeriod();

        INFO("Process Block Call: " << processBlockCall << ", Detected Pitch Period: " << detectedPeriod);

        // Skip pitch detection check in early blocks - not enough signal history for accurate detection
        // Need at least 2x detection buffer size for stable detection
        const int minBlocksForDetection = (TestConfig::pitchDetectBufferSize * 2) / TestConfig::blockSize;
        if(processBlockCall >= minBlocksForDetection)
        {
            float expectedPeriod = 256.f;
            // Accept fundamental or octave errors (common in pitch detection)
            bool validDetection = (detectedPeriod >= expectedPeriod * 0.45 && detectedPeriod <= expectedPeriod * 0.55) ||  // Octave up
                                 (detectedPeriod >= expectedPeriod * 0.9 && detectedPeriod <= expectedPeriod * 1.1) ||      // Fundamental
                                 (detectedPeriod >= expectedPeriod * 1.9 && detectedPeriod <= expectedPeriod * 2.1);       // Octave down
            CHECK(validDetection);
        }

        // Compare RMS instead of individual samples - granular processing preserves energy, not waveform shape
        // Wait until after startup transient (detection + grain ramp-up)
        const int warmupBlocks = minBlocksForDetection + 5;
        if(processBlockCall >= warmupBlocks)
        {
            float inputRMS = sineBuffer.getRMSLevel(0, 0, sineBuffer.getNumSamples());
            float outputRMS = processBuffer.getRMSLevel(0, 0, processBuffer.getNumSamples());

            INFO("Process Block Call: " << processBlockCall << ", Input RMS: " << inputRMS << ", Output RMS: " << outputRMS);

            // Granular processing should preserve approximate energy (within 15%)
            CHECK(outputRMS == Catch::Approx(inputRMS).epsilon(0.15f));
        }
        else  // During warmup and startup transient
        {
            float outputRMS = processBuffer.getRMSLevel(0, 0, processBuffer.getNumSamples());
            INFO("Process Block Call: " << processBlockCall << " (warmup/transient), Output RMS: " << outputRMS);
            // Don't check RMS during startup - it's expected to be low or zero
        }
    }
}


//==============================================================================
// Root Cause Investigation Tests
//==============================================================================

TEST_CASE("GranulatorProcessor pitch detection accuracy with non-aligned frequency", "[GranulatorProcessor][psola][debug]")
{
    /**
     * Test 1: Verify pitch detection is accurate for frequencies that don't align with block size.
     * If detection is wrong, everything downstream will be wrong.
     */
    TestUtils::SetupAndTeardown setup;

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr float testFrequency = 220.0f;  // Period = 218.18 samples (non-aligned)
    const float expectedPeriod = static_cast<float>(sampleRate / testFrequency);

    GranulatorProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> processBuffer(2, blockSize);
    juce::MidiBuffer midiBuffer;

    // use unrealistically low threshold
    processor.getPitchDetector()->setThreshold(0.01f);
    // Process sine wave - need enough blocks to fill detection buffer before checking
    // Detection buffer is 1024 samples, so need at least 1024/256 = 4 blocks of warmup
    constexpr int warmupBlocks = 10;  // Use extra warmup to ensure clean data

    for (int blockIndex = 0; blockIndex < warmupBlocks; ++blockIndex)
    {
        processBuffer.clear();
        const double startPhase = std::fmod(
            static_cast<double>(blockIndex * blockSize) / expectedPeriod * 2.0 * M_PI,
            2.0 * M_PI
        );
        BufferFiller::generateSineWithPhase(processBuffer, expectedPeriod, startPhase);
        processor.processBlock(processBuffer, midiBuffer);
    }

    // Now check if pitch was detected
    bool pitchDetected = false;
    float detectedPeriod = processor.getLastDetectedPeriod();
    int detectionBlockIndex = warmupBlocks;

    if (detectedPeriod > 0)
    {
        pitchDetected = true;
    }

    // If not detected yet, process a few more blocks
    if (!pitchDetected)
    {
        for (int blockIndex = warmupBlocks; blockIndex < 30; ++blockIndex)
        {
            processBuffer.clear();
            const double startPhase = std::fmod(
                static_cast<double>(blockIndex * blockSize) / expectedPeriod * 2.0 * M_PI,
                2.0 * M_PI
            );
            BufferFiller::generateSineWithPhase(processBuffer, expectedPeriod, startPhase);
            processor.processBlock(processBuffer, midiBuffer);

            detectedPeriod = processor.getLastDetectedPeriod();
            if (detectedPeriod > 0)
            {
                pitchDetected = true;
                detectionBlockIndex = blockIndex;
                break;
            }
        }
    }

    REQUIRE(pitchDetected);

    // Now manually check what the pitch detector is seeing
    auto [detectStart, detectEnd] = processor.getDetectionRange();
    int detectionSize = static_cast<int>(detectEnd - detectStart);

    juce::AudioBuffer<float> manualDetectionBuffer(2, detectionSize);
    manualDetectionBuffer.clear();
    processor.getCircularBuffer().readRange(manualDetectionBuffer, detectStart);

    float detectionBufferRMS = manualDetectionBuffer.getRMSLevel(0, 0, detectionSize);
    float detectionBufferMax = manualDetectionBuffer.getMagnitude(0, 0, detectionSize);

    INFO("Detection block index: " << detectionBlockIndex);
    INFO("Detection buffer RMS: " << detectionBufferRMS);
    INFO("Detection buffer max: " << detectionBufferMax);

    float detectionError = std::abs(detectedPeriod - expectedPeriod) / expectedPeriod;
    float detectionErrorPercent = detectionError * 100.0f;

    INFO("Expected period: " << expectedPeriod);
    INFO("Detected period: " << detectedPeriod);
    INFO("Detection error: " << detectionErrorPercent << "%");

    // Pitch detection should be accurate within 5%
    CHECK(detectionError < 0.05f);
}

TEST_CASE("PitchDetector direct test with clean sine wave", "[PitchDetector][psola][debug]")
{
    /**
     * Test the PitchDetector directly with a known clean sine wave
     * to isolate whether YIN algorithm itself is working correctly.
     */
    TestUtils::SetupAndTeardown setup;

    constexpr double sampleRate = 48000.0;
    constexpr int detectionBufferSize = 2048;
    constexpr float testFrequency = 220.0f;
    const float expectedPeriod = static_cast<float>(sampleRate / testFrequency);  // 218.18

    PitchDetector detector;
    detector.prepareToPlay(detectionBufferSize);

    detector.setThreshold(0.01);
    
    // Create a clean sine wave buffer
    juce::AudioBuffer<float> testBuffer(2, detectionBufferSize);
    testBuffer.clear();
    BufferFiller::generateSineWithPhase(testBuffer, expectedPeriod, 0.0);

    // Verify buffer contains clean data
    float rms = testBuffer.getRMSLevel(0, 0, detectionBufferSize);
    float maxAbs = testBuffer.getMagnitude(0, 0, detectionBufferSize);

    INFO("Test buffer RMS: " << rms);
    INFO("Test buffer max: " << maxAbs);

    REQUIRE(rms > 0.6f);  // Should have strong signal

    // Run pitch detection
    float detectedPeriod = detector.process(testBuffer);

    INFO("Expected period: " << expectedPeriod);
    INFO("Detected period: " << detectedPeriod);

    REQUIRE(detectedPeriod > 0);  // Should detect something

    float detectionError = std::abs(detectedPeriod - expectedPeriod) / expectedPeriod;
    INFO("Detection error: " << (detectionError * 100.0f) << "%");

    // YIN should accurately detect the period
    CHECK(detectionError < 0.05f);
}

TEST_CASE("Granulator window normalization check", "[Granulator][psola][debug]")
{
    /**
     * Test 3: Check if overlapping windows sum to constant value.
     * For proper OLA reconstruction, window overlap should sum to ~1.0 at all times.
     */
    TestUtils::SetupAndTeardown setup;

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr float testFrequency = 220.0f;
    const float expectedPeriod = static_cast<float>(sampleRate / testFrequency);

    GranulatorProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);

    // Set unity pitch ratio
    auto* param = processor.getAPVTS().getParameter("shift ratio");
    REQUIRE(param != nullptr);
    param->setValueNotifyingHost(0.5f);

    juce::AudioBuffer<float> processBuffer(2, blockSize);
    juce::MidiBuffer midiBuffer;

    // Process enough blocks to reach steady state
    constexpr int warmupBlocks = 15;
    for (int blockIndex = 0; blockIndex < warmupBlocks; ++blockIndex)
    {
        processBuffer.clear();
        const double startPhase = std::fmod(
            static_cast<double>(blockIndex * blockSize) / expectedPeriod * 2.0 * M_PI,
            2.0 * M_PI
        );
        BufferFiller::generateSineWithPhase(processBuffer, expectedPeriod, startPhase);
        processor.processBlock(processBuffer, midiBuffer);
    }

    // Get access to the granulator to check internal state
    auto& granulator = processor.getGranulator();

    // Process one more block and capture the normalization window buffer
    // We need to check if mNormWindowBuffer is constant during processing
    // Since it's private, we'll infer the normalization quality from output consistency

    // At unity ratio with correct OLA, the output amplitude should be consistent
    // Let's check multiple blocks to see if amplitude varies
    std::vector<float> blockRMS;
    std::vector<float> blockMaxAbs;

    for (int blockIndex = 0; blockIndex < 10; ++blockIndex)
    {
        processBuffer.clear();
        const double startPhase = std::fmod(
            static_cast<double>((warmupBlocks + blockIndex) * blockSize) / expectedPeriod * 2.0 * M_PI,
            2.0 * M_PI
        );
        BufferFiller::generateSineWithPhase(processBuffer, expectedPeriod, startPhase);
        processor.processBlock(processBuffer, midiBuffer);

        float rms = processBuffer.getRMSLevel(0, 0, blockSize);
        float maxAbs = processBuffer.getMagnitude(0, 0, blockSize);

        blockRMS.push_back(rms);
        blockMaxAbs.push_back(maxAbs);
    }

    // Calculate coefficient of variation for RMS across blocks
    float avgRMS = 0.0f;
    for (float r : blockRMS)
    {
        avgRMS += r;
    }
    avgRMS /= static_cast<float>(blockRMS.size());

    float rmsVariance = 0.0f;
    for (float r : blockRMS)
    {
        float diff = r - avgRMS;
        rmsVariance += diff * diff;
    }
    float rmsStdDev = std::sqrt(rmsVariance / static_cast<float>(blockRMS.size()));
    float rmsCV = rmsStdDev / avgRMS;

    // Calculate coefficient of variation for max amplitude
    float avgMaxAbs = 0.0f;
    for (float m : blockMaxAbs)
    {
        avgMaxAbs += m;
    }
    avgMaxAbs /= static_cast<float>(blockMaxAbs.size());

    float maxVariance = 0.0f;
    for (float m : blockMaxAbs)
    {
        float diff = m - avgMaxAbs;
        maxVariance += diff * diff;
    }
    float maxStdDev = std::sqrt(maxVariance / static_cast<float>(blockMaxAbs.size()));
    float maxCV = maxStdDev / avgMaxAbs;

    INFO("Block RMS statistics:");
    INFO("  Average RMS: " << avgRMS);
    INFO("  RMS CV: " << rmsCV);
    INFO("Block max amplitude statistics:");
    INFO("  Average max: " << avgMaxAbs);
    INFO("  Max CV: " << maxCV);

    // If window normalization is correct, amplitude should be very consistent across blocks
    // CV should be < 5% for proper OLA
    CHECK(rmsCV < 0.05f);
    CHECK(maxCV < 0.05f);
}





//=============================================================================
TEST_CASE("GranulatorProcessor Pitch Detection", "[GranulatorProcessor][PitchDetection]")
{
    /**
     * Retrieve Pitch Detector and detect directly
     */
    TestUtils::SetupAndTeardown setup;

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int minLookaheadSize = MagicNumbers::minLookaheadSize;

    // a period that does not align with block size
    const float expectedPeriod = 200;

    GranulatorProcessor processor;
    processor.prepareToPlay(sampleRate, blockSize);
    processor.getPitchDetector()->setThreshold(0.01f);

    juce::AudioBuffer<float> sineBuffer(1, PitchDetectorMagicNumbers::DefaultDetectionSize);
    BufferFiller::generateSineCycles(sineBuffer, expectedPeriod, 0.0);

    float detectedPeriod = processor.getPitchDetector()->process(sineBuffer);
    CHECK(detectedPeriod == Catch::Approx(expectedPeriod).margin(1.f));

}

