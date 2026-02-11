/**
 * test_TD_PitchDetector.cpp
 * Tests for TD_PitchDetector class (FFT-based autocorrelation)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../SOURCE/PITCH/TD_PitchDetector.h"
#include "../SOURCE/BufferFiller.h"
#include <cmath>

/**
 * Helper to calculate expected period for a frequency.
 */
static float getExpectedPeriod(float frequency, double sampleRate)
{
    return static_cast<float>(sampleRate / frequency);
}

//=======================================
TEST_CASE("TD_PitchDetector - Basic Construction", "[TD_PitchDetector]")
{
    TD_PitchDetector detector;

    SECTION("Initial state")
    {
        REQUIRE(detector.getCurrentPeriod() == -1.0f);
    }

    SECTION("Prepare")
    {
        detector.prepare(44100.0);
        REQUIRE(detector.getCurrentPeriod() == -1.0f);
        REQUIRE(detector.getMinPeriod() > 0);
        REQUIRE(detector.getMaxPeriod() > detector.getMinPeriod());
    }

    SECTION("Frequency range configuration")
    {
        detector.prepare(44100.0);
        detector.setFrequencyRange(100.0f, 500.0f);

        int expectedMinPeriod = static_cast<int>(44100.0 / 500.0f);
        int expectedMaxPeriod = static_cast<int>(44100.0 / 100.0f);

        REQUIRE(detector.getMinPeriod() == expectedMinPeriod);
        REQUIRE(detector.getMaxPeriod() == expectedMaxPeriod);
    }
}

//=======================================
TEST_CASE("TD_PitchDetector - Sine Wave Detection", "[TD_PitchDetector]")
{
    const double sampleRate = 44100.0;
    const int bufferSize = 2048;

    TD_PitchDetector detector;
    detector.prepare(sampleRate);

    SECTION("Detect 440 Hz (A4)")
    {
        const float frequency = 440.0f;
        const float expectedPeriod = getExpectedPeriod(frequency, sampleRate);

        juce::AudioBuffer<float> testBuffer(1, bufferSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));

        float detectedPeriod = detector.process(testBuffer);

        REQUIRE(detectedPeriod > 0.0f);
        REQUIRE(detector.getCurrentPeriod() == detectedPeriod);

        // Allow 5% error tolerance for FFT-based detection
        float error = std::abs(detectedPeriod - expectedPeriod);
        REQUIRE(error < expectedPeriod * 0.05f);
    }

    SECTION("Detect 220 Hz (A3)")
    {
        const float frequency = 220.0f;
        const float expectedPeriod = getExpectedPeriod(frequency, sampleRate);

        juce::AudioBuffer<float> testBuffer(1, bufferSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));

        float detectedPeriod = detector.process(testBuffer);

        REQUIRE(detectedPeriod > 0.0f);

        float error = std::abs(detectedPeriod - expectedPeriod);
        REQUIRE(error < expectedPeriod * 0.05f);
    }

    SECTION("Detect 100 Hz (low frequency)")
    {
        detector.setFrequencyRange(80.0f, 400.0f);

        const float frequency = 100.0f;
        const float expectedPeriod = getExpectedPeriod(frequency, sampleRate);

        juce::AudioBuffer<float> testBuffer(1, bufferSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));

        float detectedPeriod = detector.process(testBuffer);

        REQUIRE(detectedPeriod > 0.0f);

        float error = std::abs(detectedPeriod - expectedPeriod);
        REQUIRE(error < expectedPeriod * 0.05f);
    }
}

//=======================================
TEST_CASE("TD_PitchDetector - Multiple Detections", "[TD_PitchDetector]")
{
    const double sampleRate = 44100.0;
    const int bufferSize = 2048;

    TD_PitchDetector detector;
    detector.prepare(sampleRate);

    SECTION("Consecutive detections on same frequency")
    {
        const float frequency = 440.0f;
        const float expectedPeriod = getExpectedPeriod(frequency, sampleRate);

        juce::AudioBuffer<float> testBuffer(1, bufferSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));

        float period1 = detector.process(testBuffer);
        float period2 = detector.process(testBuffer);

        REQUIRE(period1 > 0.0f);
        REQUIRE(period2 > 0.0f);

        // Both detections should be similar
        float diff = std::abs(period1 - period2);
        REQUIRE(diff < expectedPeriod * 0.05f);
    }

    SECTION("Different frequencies detected correctly")
    {
        juce::AudioBuffer<float> testBuffer(1, bufferSize);

        // First frequency
        float period220 = getExpectedPeriod(220.0f, sampleRate);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(period220));
        float period1 = detector.process(testBuffer);

        // Second frequency
        float period440 = getExpectedPeriod(440.0f, sampleRate);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(period440));
        float period2 = detector.process(testBuffer);

        REQUIRE(period1 > 0.0f);
        REQUIRE(period2 > 0.0f);

        // 440 Hz should have roughly half the period of 220 Hz
        REQUIRE(period2 < period1);
        REQUIRE(std::abs(period1 / period2 - 2.0f) < 0.2f);
    }
}

//=======================================
TEST_CASE("TD_PitchDetector - Amplitude Independence", "[TD_PitchDetector]")
{
    const double sampleRate = 44100.0;
    const int bufferSize = 2048;
    const float frequency = 440.0f;
    const float expectedPeriod = getExpectedPeriod(frequency, sampleRate);

    TD_PitchDetector detector;
    detector.prepare(sampleRate);

    SECTION("Quiet signal (amplitude 0.1)")
    {
        juce::AudioBuffer<float> testBuffer(1, bufferSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));
        testBuffer.applyGain(0.1f);

        float detectedPeriod = detector.process(testBuffer);

        REQUIRE(detectedPeriod > 0.0f);

        float error = std::abs(detectedPeriod - expectedPeriod);
        REQUIRE(error < expectedPeriod * 0.05f);
    }

    SECTION("Loud signal (amplitude 1.0)")
    {
        juce::AudioBuffer<float> testBuffer(1, bufferSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));

        float detectedPeriod = detector.process(testBuffer);

        REQUIRE(detectedPeriod > 0.0f);

        float error = std::abs(detectedPeriod - expectedPeriod);
        REQUIRE(error < expectedPeriod * 0.05f);
    }
}

//=======================================
TEST_CASE("TD_PitchDetector - Buffer Size Handling", "[TD_PitchDetector]")
{
    const double sampleRate = 44100.0;
    const float frequency = 440.0f;

    TD_PitchDetector detector;
    detector.prepare(sampleRate);

    SECTION("Small buffer (512 samples)")
    {
        const float expectedPeriod = getExpectedPeriod(frequency, sampleRate);
        juce::AudioBuffer<float> testBuffer(1, 512);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));

        float detectedPeriod = detector.process(testBuffer);
        REQUIRE(detectedPeriod > 0.0f);
    }

    SECTION("Medium buffer (2048 samples)")
    {
        const float expectedPeriod = getExpectedPeriod(frequency, sampleRate);
        juce::AudioBuffer<float> testBuffer(1, 2048);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));

        float detectedPeriod = detector.process(testBuffer);
        REQUIRE(detectedPeriod > 0.0f);
    }

    SECTION("Large buffer (4096 samples)")
    {
        const float expectedPeriod = getExpectedPeriod(frequency, sampleRate);
        juce::AudioBuffer<float> testBuffer(1, 4096);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));

        float detectedPeriod = detector.process(testBuffer);
        REQUIRE(detectedPeriod > 0.0f);
    }
}

//=======================================
TEST_CASE("TD_PitchDetector - Edge Cases", "[TD_PitchDetector]")
{
    const double sampleRate = 44100.0;
    TD_PitchDetector detector;
    detector.prepare(sampleRate);

    SECTION("Silence should not crash")
    {
        juce::AudioBuffer<float> testBuffer(1, 2048);
        testBuffer.clear();

        // Should not crash, may or may not detect a period
        float detectedPeriod = detector.process(testBuffer);
        (void)detectedPeriod; // May be -1 or some value
    }

    SECTION("Noise should not crash")
    {
        juce::AudioBuffer<float> testBuffer(1, 2048);
        juce::Random random;

        for (int i = 0; i < testBuffer.getNumSamples(); ++i)
        {
            testBuffer.setSample(0, i, random.nextFloat() * 2.0f - 1.0f);
        }

        // Should not crash
        float detectedPeriod = detector.process(testBuffer);
        (void)detectedPeriod;
    }
}
