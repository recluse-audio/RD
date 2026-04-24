/**
 * test_FFT_PitchDetector.cpp
 * Tests for FFT_PitchDetector class (FFT-based autocorrelation)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../SOURCE/PITCH/FFT_PitchDetector.h"
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
TEST_CASE("FFT_PitchDetector - Basic Construction", "[FFT_PitchDetector]")
{
    FFT_PitchDetector detector;

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
TEST_CASE("FFT_PitchDetector - Sine Wave Detection", "[FFT_PitchDetector]")
{
    const double sampleRate = 44100.0;
    const int bufferSize = 2048;

    FFT_PitchDetector detector;
    detector.prepare(sampleRate);

    SECTION("Detect 440 Hz (A4)")
    {
        const float frequency = 440.0f;
        const float expectedPeriod = getExpectedPeriod(frequency, sampleRate);

        juce::AudioBuffer<float> testBuffer(1, bufferSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));

        float detectedPeriod = detector.process(testBuffer);

        INFO("Expected period: " << expectedPeriod << ", Detected period: " << detectedPeriod);
        REQUIRE(detectedPeriod > 0.0f);
        REQUIRE(detector.getCurrentPeriod() == detectedPeriod);

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
TEST_CASE("FFT_PitchDetector - Multiple Detections", "[FFT_PitchDetector]")
{
    const double sampleRate = 44100.0;
    const int bufferSize = 2048;

    FFT_PitchDetector detector;
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

        float diff = std::abs(period1 - period2);
        REQUIRE(diff < expectedPeriod * 0.05f);
    }

    SECTION("Different frequencies detected correctly")
    {
        juce::AudioBuffer<float> testBuffer(1, bufferSize);

        float period220 = getExpectedPeriod(220.0f, sampleRate);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(period220));
        float period1 = detector.process(testBuffer);

        float period440 = getExpectedPeriod(440.0f, sampleRate);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(period440));
        float period2 = detector.process(testBuffer);

        REQUIRE(period1 > 0.0f);
        REQUIRE(period2 > 0.0f);

        REQUIRE(period2 < period1);
        REQUIRE(std::abs(period1 / period2 - 2.0f) < 0.2f);
    }
}

//=======================================
TEST_CASE("FFT_PitchDetector - Amplitude Independence", "[FFT_PitchDetector]")
{
    const double sampleRate = 44100.0;
    const int bufferSize = 2048;
    const float frequency = 440.0f;
    const float expectedPeriod = getExpectedPeriod(frequency, sampleRate);

    FFT_PitchDetector detector;
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
TEST_CASE("FFT_PitchDetector - Buffer Size Handling", "[FFT_PitchDetector]")
{
    const double sampleRate = 44100.0;
    const float frequency = 440.0f;

    FFT_PitchDetector detector;
    detector.prepare(sampleRate);

    SECTION("Small buffer (512 samples)")
    {
        // A 512-sample window cannot fit 2 periods at the default 80 Hz floor
        // (2 * 44100/80 = 1102 samples). Narrow the range so the detector's
        // "window must fit 2 periods at mMinHz" guard is satisfied.
        detector.setFrequencyRange(200.0f, 1000.0f);

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
TEST_CASE("FFT_PitchDetector - Edge Cases", "[FFT_PitchDetector]")
{
    const double sampleRate = 44100.0;
    FFT_PitchDetector detector;
    detector.prepare(sampleRate);

    SECTION("Silence should not crash")
    {
        juce::AudioBuffer<float> testBuffer(1, 2048);
        testBuffer.clear();

        float detectedPeriod = detector.process(testBuffer);
        (void)detectedPeriod;
    }

    SECTION("Noise should not crash")
    {
        juce::AudioBuffer<float> testBuffer(1, 2048);
        juce::Random random;

        for (int i = 0; i < testBuffer.getNumSamples(); ++i)
        {
            testBuffer.setSample(0, i, random.nextFloat() * 2.0f - 1.0f);
        }

        float detectedPeriod = detector.process(testBuffer);
        (void)detectedPeriod;
    }
}
