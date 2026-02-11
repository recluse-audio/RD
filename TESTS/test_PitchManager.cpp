/**
 * test_PitchManager.cpp
 * Tests for PitchManager class
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../SOURCE/PITCH/PitchManager.h"
#include "../SOURCE/CircularBuffer.h"
#include "../SOURCE/BufferFiller.h"
#include <cmath>

//=======================================
TEST_CASE("PitchManager - Basic Construction", "[PitchManager]")
{
    PitchManager manager;

    SECTION("Initial state")
    {
        REQUIRE(manager.getCurrentPeriod() == -1.0f);
        REQUIRE(manager.getAbsoluteSampleCount() == 0);
        REQUIRE(!manager.isReadyForDetection());
    }

    SECTION("Prepare")
    {
        manager.prepare(44100.0);
        REQUIRE(manager.getCurrentPeriod() == -1.0f);
        REQUIRE(manager.getAbsoluteSampleCount() == 0);
        REQUIRE(!manager.isReadyForDetection());
    }

    SECTION("Reset")
    {
        manager.prepare(44100.0);
        manager.reset();
        REQUIRE(manager.getCurrentPeriod() == -1.0f);
        REQUIRE(manager.getAbsoluteSampleCount() == 0);
    }
}

//=======================================
TEST_CASE("PitchManager - Sample Accumulation", "[PitchManager]")
{
    const double sampleRate = 44100.0;
    const int detectionWindowSize = 2048;

    PitchManager manager;
    manager.prepare(sampleRate, detectionWindowSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(1, 8192);

    SECTION("Accumulates samples until detection window is full")
    {
        const float frequency = 440.0f;
        const float period = sampleRate / frequency;

        // Create smaller buffers that need to be accumulated
        const int bufferSize = 512;
        juce::AudioBuffer<float> testBuffer(1, bufferSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(period));

        // Process 3 buffers (3 * 512 = 1536 samples, less than 2048)
        bool detected1 = manager.process(testBuffer, circularBuffer);
        REQUIRE(!detected1);
        REQUIRE(manager.getAbsoluteSampleCount() == bufferSize);

        bool detected2 = manager.process(testBuffer, circularBuffer);
        REQUIRE(!detected2);
        REQUIRE(manager.getAbsoluteSampleCount() == bufferSize * 2);

        bool detected3 = manager.process(testBuffer, circularBuffer);
        REQUIRE(!detected3);
        REQUIRE(manager.getAbsoluteSampleCount() == bufferSize * 3);

        // Process 4th buffer (4 * 512 = 2048 samples, should trigger detection)
        bool detected4 = manager.process(testBuffer, circularBuffer);
        REQUIRE(detected4);
        REQUIRE(manager.getAbsoluteSampleCount() == bufferSize * 4);
        REQUIRE(manager.getCurrentPeriod() > 0.0f);
    }

    SECTION("Handles buffers larger than detection window")
    {
        const float frequency = 440.0f;
        const float period = sampleRate / frequency;

        // Create a buffer larger than detection window
        const int bufferSize = 4096;  // Larger than 2048
        juce::AudioBuffer<float> testBuffer(1, bufferSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(period));

        bool detected = manager.process(testBuffer, circularBuffer);
        REQUIRE(detected);
        REQUIRE(manager.getCurrentPeriod() > 0.0f);
        REQUIRE(manager.getAbsoluteSampleCount() == bufferSize);
    }
}

//=======================================
TEST_CASE("PitchManager - Pitch Detection", "[PitchManager]")
{
    const double sampleRate = 44100.0;
    const int detectionWindowSize = 2048;

    PitchManager manager;
    manager.prepare(sampleRate, detectionWindowSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(1, 8192);

    SECTION("Detects pitch from sine wave")
    {
        const float frequency = 440.0f;
        const float expectedPeriod = sampleRate / frequency;

        juce::AudioBuffer<float> testBuffer(1, detectionWindowSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));

        bool detected = manager.process(testBuffer, circularBuffer);
        REQUIRE(detected);

        float detectedPeriod = manager.getCurrentPeriod();
        REQUIRE(detectedPeriod > 0.0f);

        // Allow 5% error tolerance
        float error = std::abs(detectedPeriod - expectedPeriod);
        REQUIRE(error < expectedPeriod * 0.05f);
    }

    SECTION("Updates period on each detection")
    {
        const float frequency1 = 220.0f;
        const float period1 = sampleRate / frequency1;

        juce::AudioBuffer<float> buffer1(1, detectionWindowSize);
        BufferFiller::generateSineCycles(buffer1, static_cast<int>(period1));

        bool detected1 = manager.process(buffer1, circularBuffer);
        REQUIRE(detected1);
        float detectedPeriod1 = manager.getCurrentPeriod();

        // Process second frequency
        const float frequency2 = 440.0f;
        const float period2 = sampleRate / frequency2;

        juce::AudioBuffer<float> buffer2(1, detectionWindowSize);
        BufferFiller::generateSineCycles(buffer2, static_cast<int>(period2));

        bool detected2 = manager.process(buffer2, circularBuffer);
        REQUIRE(detected2);
        float detectedPeriod2 = manager.getCurrentPeriod();

        // Periods should be different
        REQUIRE(detectedPeriod1 != detectedPeriod2);
        REQUIRE(detectedPeriod2 < detectedPeriod1);  // Higher freq = shorter period
    }
}

//=======================================
TEST_CASE("PitchManager - Pitch Mark Finding", "[PitchManager]")
{
    const double sampleRate = 44100.0;
    const int detectionWindowSize = 2048;
    const float frequency = 440.0f;
    const float expectedPeriod = sampleRate / frequency;

    PitchManager manager;
    manager.prepare(sampleRate, detectionWindowSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(1, 8192);

    SECTION("Cannot find marks without detected period")
    {
        juce::Range<juce::int64> searchRange(0, 1000);
        juce::int64 mark = manager.findPitchMark(circularBuffer, searchRange, false);
        REQUIRE(mark == -1);
    }

    SECTION("Finds pitch marks after detection")
    {
        // First, run detection
        juce::AudioBuffer<float> testBuffer(1, detectionWindowSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));
        circularBuffer.pushBuffer(testBuffer);

        bool detected = manager.process(testBuffer, circularBuffer);
        REQUIRE(detected);
        REQUIRE(manager.getCurrentPeriod() > 0.0f);

        // Now find a pitch mark
        const juce::int64 searchEnd = manager.getAbsoluteSampleCount();
        const juce::int64 searchStart = searchEnd - static_cast<juce::int64>(expectedPeriod);
        juce::Range<juce::int64> searchRange(searchStart, searchEnd);

        juce::int64 mark = manager.findPitchMark(circularBuffer, searchRange, false);
        REQUIRE(mark >= searchStart);
        REQUIRE(mark < searchEnd);
    }

    SECTION("Uses prediction for consecutive marks")
    {
        // Process audio to get detection
        juce::AudioBuffer<float> testBuffer(1, detectionWindowSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));
        circularBuffer.pushBuffer(testBuffer);

        manager.process(testBuffer, circularBuffer);

        // Find first mark
        juce::int64 searchEnd = manager.getAbsoluteSampleCount();
        juce::int64 searchStart = searchEnd - static_cast<juce::int64>(expectedPeriod);
        juce::Range<juce::int64> searchRange1(searchStart, searchEnd);

        juce::int64 firstMark = manager.findPitchMark(circularBuffer, searchRange1, false);
        REQUIRE(firstMark >= 0);

        // Add more audio
        circularBuffer.pushBuffer(testBuffer);
        manager.process(testBuffer, circularBuffer);

        // Find second mark with prediction
        searchEnd = manager.getAbsoluteSampleCount();
        searchStart = searchEnd - static_cast<juce::int64>(expectedPeriod);
        juce::Range<juce::int64> searchRange2(searchStart, searchEnd);

        juce::int64 secondMark = manager.findPitchMark(circularBuffer, searchRange2, true);
        REQUIRE(secondMark >= 0);
        REQUIRE(secondMark > firstMark);
    }
}

//=======================================
TEST_CASE("PitchManager - Absolute Sample Counter", "[PitchManager]")
{
    const double sampleRate = 44100.0;
    const int detectionWindowSize = 2048;

    PitchManager manager;
    manager.prepare(sampleRate, detectionWindowSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(1, 8192);

    SECTION("Tracks total samples processed")
    {
        const float frequency = 440.0f;
        const float period = sampleRate / frequency;

        juce::AudioBuffer<float> buffer1(1, 512);
        BufferFiller::generateSineCycles(buffer1, static_cast<int>(period));
        manager.process(buffer1, circularBuffer);
        REQUIRE(manager.getAbsoluteSampleCount() == 512);

        juce::AudioBuffer<float> buffer2(1, 1024);
        BufferFiller::generateSineCycles(buffer2, static_cast<int>(period));
        manager.process(buffer2, circularBuffer);
        REQUIRE(manager.getAbsoluteSampleCount() == 512 + 1024);

        juce::AudioBuffer<float> buffer3(1, 2048);
        BufferFiller::generateSineCycles(buffer3, static_cast<int>(period));
        manager.process(buffer3, circularBuffer);
        REQUIRE(manager.getAbsoluteSampleCount() == 512 + 1024 + 2048);
    }

    SECTION("Resets counter on reset()")
    {
        const float frequency = 440.0f;
        const float period = sampleRate / frequency;

        juce::AudioBuffer<float> testBuffer(1, 2048);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(period));
        manager.process(testBuffer, circularBuffer);

        REQUIRE(manager.getAbsoluteSampleCount() > 0);

        manager.reset();
        REQUIRE(manager.getAbsoluteSampleCount() == 0);
    }
}

//=======================================
TEST_CASE("PitchManager - Configuration Access", "[PitchManager]")
{
    PitchManager manager;
    manager.prepare(44100.0);

    SECTION("Can access pitch detector for configuration")
    {
        TD_PitchDetector& detector = manager.getPitchDetector();
        detector.setFrequencyRange(100.0f, 500.0f);
        REQUIRE(detector.getMinPeriod() > 0);
    }

    SECTION("Can access pitch marker for configuration")
    {
        PitchMarker& marker = manager.getPitchMarker();
        marker.reset();
        REQUIRE(marker.getLastMark() == -1);
    }
}
