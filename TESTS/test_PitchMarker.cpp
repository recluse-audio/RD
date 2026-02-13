/**
 * test_PitchMarker.cpp
 * Tests for PitchMarker class
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../SOURCE/PITCH/PitchMarker.h"
#include "../SOURCE/PITCH/PitchMark.h"
#include "../SOURCE/CircularBuffer.h"
#include "../SOURCE/BufferFiller.h"
#include <cmath>

//=======================================
TEST_CASE("PitchMark - Basic Functionality", "[PitchMark]")
{
    SECTION("Default constructor creates invalid mark")
    {
        PitchMark mark;
        REQUIRE(!mark.isValid());
        REQUIRE(mark.mark == -1);
        REQUIRE(mark.rangeStart == -1);
        REQUIRE(mark.rangeEnd == -1);
    }

    SECTION("Constructor with period sets range correctly")
    {
        const juce::int64 markPosition = 1000;
        const float period = 100.0f;

        PitchMark mark(markPosition, period);

        REQUIRE(mark.isValid());
        REQUIRE(mark.mark == markPosition);
        REQUIRE(mark.rangeStart == markPosition - 100);  // mark - period
        REQUIRE(mark.rangeEnd == markPosition + 100 - 1);  // mark + period - 1
        REQUIRE(mark.getRangeLength() == 200);  // 2 * period
    }

    SECTION("getRange returns correct juce::Range")
    {
        const juce::int64 markPosition = 500;
        const float period = 50.0f;

        PitchMark mark(markPosition, period);
        juce::Range<juce::int64> range = mark.getRange();

        REQUIRE(range.getStart() == 450);  // mark - period
        REQUIRE(range.getEnd() == 550);    // mark + period - 1 (Range is exclusive at end)
        REQUIRE(range.getLength() == 100); // Full range length
    }
}

//=======================================
TEST_CASE("PitchMarker - Basic Construction", "[PitchMarker]")
{
    PitchMarker marker;

    SECTION("Initial state")
    {
        REQUIRE(marker.getLastMark() == -1);
        REQUIRE(marker.getPredictedNextMark() == -1);
        REQUIRE(marker.getNumStoredMarks() == 0);
    }

    SECTION("Prepare")
    {
        const double sampleRate = 44100.0;
        const int detectionWindowSize = 2048;
        marker.prepare(sampleRate, detectionWindowSize);
        REQUIRE(marker.getLastMark() == -1);
        REQUIRE(marker.getPredictedNextMark() == -1);
        REQUIRE(marker.getNumStoredMarks() == 0);
    }

    SECTION("Reset")
    {
        marker.reset();
        REQUIRE(marker.getLastMark() == -1);
        REQUIRE(marker.getPredictedNextMark() == -1);
        REQUIRE(marker.getNumStoredMarks() == 0);
    }
}

//=======================================
TEST_CASE("PitchMarker - Find Mark in Sine Wave", "[PitchMarker]")
{
    const double sampleRate = 44100.0;
    const float frequency = 440.0f; // A4
    const float expectedPeriod = static_cast<float>(sampleRate / frequency); // ~100 samples

    PitchMarker marker;
    marker.prepare(sampleRate, 2048);

    CircularBuffer circularBuffer;

    const int bufferSize = 4096;
    circularBuffer.setSize(1, bufferSize);

    // Generate sine wave and push to circular buffer
    juce::AudioBuffer<float> testBuffer(1, bufferSize);
    BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));
    circularBuffer.pushBuffer(testBuffer);

    SECTION("Find first mark without prediction")
    {
        // Search the last period of the buffer
        const juce::int64 searchEnd = bufferSize;
        const juce::int64 searchStart = searchEnd - static_cast<juce::int64>(expectedPeriod);
        juce::Range<juce::int64> searchRange(searchStart, searchEnd);

        juce::int64 foundMark = marker.doPitchMarking(circularBuffer, searchRange, expectedPeriod, bufferSize, false); // Don't use prediction

        // Mark should be within search range
        REQUIRE(foundMark >= searchStart);
        REQUIRE(foundMark < searchEnd);

        // For a sine wave starting at phase 0, peak occurs at period/4
        // The detected mark should be near a peak (value close to 1.0)
        int wrappedIndex = static_cast<int>(foundMark % bufferSize);
        float detectedSample = circularBuffer.getBuffer().getSample(0, wrappedIndex);
        REQUIRE(detectedSample > 0.9f); // Should be near peak

        // Prediction should now be set
        REQUIRE(marker.getPredictedNextMark() > foundMark);
        REQUIRE(marker.getLastMark() == foundMark);

        // Mark should be stored in FIFO
        REQUIRE(marker.getNumStoredMarks() == 1);
        const PitchMark& storedMark = marker.getPitchMarks()[0];
        REQUIRE(storedMark.isValid());
        REQUIRE(storedMark.mark == foundMark);
        REQUIRE(storedMark.rangeStart == foundMark - static_cast<juce::int64>(expectedPeriod));
        REQUIRE(storedMark.rangeEnd == foundMark + static_cast<juce::int64>(expectedPeriod) - 1);
    }

    SECTION("Find consecutive marks with prediction")
    {
        // First mark
        juce::int64 searchEnd = bufferSize;
        juce::int64 searchStart = searchEnd - static_cast<juce::int64>(expectedPeriod);
        juce::Range<juce::int64> searchRange(searchStart, searchEnd);

        juce::int64 firstMark = marker.doPitchMarking(circularBuffer, searchRange, expectedPeriod, bufferSize, false);

        juce::int64 prediction = marker.getPredictedNextMark();

        // Add more audio to circular buffer (continue the sine wave)
        juce::AudioBuffer<float> nextBuffer(1, 512);
        double normalizedPhase = static_cast<double>(bufferSize % static_cast<int>(expectedPeriod)) / expectedPeriod;
        BufferFiller::generateSineCycles(nextBuffer, expectedPeriod, normalizedPhase);
        circularBuffer.pushBuffer(nextBuffer);

        // Second mark using prediction
        searchEnd = bufferSize + 512;
        searchStart = prediction - static_cast<juce::int64>(expectedPeriod / 2);
        juce::Range<juce::int64> searchRange2(searchStart, searchEnd);

        juce::int64 secondMark = marker.doPitchMarking(circularBuffer, searchRange2, expectedPeriod, bufferSize + 512, true); // Use prediction

        // Second mark should be approximately one period after first mark
        juce::int64 actualPeriod = secondMark - firstMark;
        float periodError = std::abs(static_cast<float>(actualPeriod) - expectedPeriod);
        REQUIRE(periodError < expectedPeriod * 0.15f); // Within 15% error

        // Both marks should be stored in FIFO
        REQUIRE(marker.getNumStoredMarks() == 2);
    }
}

//=======================================
TEST_CASE("PitchMarker - Correlation Refinement", "[PitchMarker]")
{
    const double sampleRate = 44100.0;
    const float frequency = 440.0f;
    const float expectedPeriod = static_cast<float>(sampleRate / frequency);

    PitchMarker marker;
    marker.prepare(sampleRate, 2048);

    CircularBuffer circularBuffer;

    const int bufferSize = 8192; // Large buffer for correlation testing
    circularBuffer.setSize(1, bufferSize);

    juce::AudioBuffer<float> testBuffer(1, bufferSize);
    BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));
    circularBuffer.pushBuffer(testBuffer);

    SECTION("Correlation activates with sufficient history")
    {
        // Search in middle of buffer where we have plenty of history
        const juce::int64 searchEnd = bufferSize - 1000;
        const juce::int64 searchStart = searchEnd - static_cast<juce::int64>(expectedPeriod);
        juce::Range<juce::int64> searchRange(searchStart, searchEnd);

        // With enough samplesProcessed (>= 2*period), correlation should activate
        juce::int64 foundMark = marker.doPitchMarking(circularBuffer, searchRange, expectedPeriod, bufferSize, false);

        REQUIRE(foundMark >= searchStart);
        REQUIRE(foundMark < searchEnd);

        // Correlation should ensure phase alignment - check peak value
        int wrappedIndex = static_cast<int>(foundMark % bufferSize);
        float detectedSample = circularBuffer.getBuffer().getSample(0, wrappedIndex);
        REQUIRE(detectedSample > 0.8f); // Should be near a peak

        // Mark should be stored
        REQUIRE(marker.getNumStoredMarks() == 1);
    }

    SECTION("Without sufficient history, correlation is skipped")
    {
        marker.reset();

        const juce::int64 searchEnd = static_cast<juce::int64>(expectedPeriod);
        const juce::int64 searchStart = 0;
        juce::Range<juce::int64> searchRange(searchStart, searchEnd);

        // Not enough history for correlation (samplesProcessed < 2*period)
        juce::int64 foundMark = marker.doPitchMarking(circularBuffer, searchRange, expectedPeriod, static_cast<juce::int64>(expectedPeriod), false);

        // Should still find a peak, just without correlation refinement
        REQUIRE(foundMark >= searchStart);
        REQUIRE(foundMark < searchEnd);

        // Mark should be stored
        REQUIRE(marker.getNumStoredMarks() == 1);
    }
}

//=======================================
TEST_CASE("PitchMarker - Prediction Behavior", "[PitchMarker]")
{
    const double sampleRate = 44100.0;
    const float frequency = 440.0f;
    const float expectedPeriod = static_cast<float>(sampleRate / frequency);

    PitchMarker marker;
    marker.prepare(sampleRate, 2048);

    CircularBuffer circularBuffer;

    const int bufferSize = 4096;
    circularBuffer.setSize(1, bufferSize);

    juce::AudioBuffer<float> testBuffer(1, bufferSize);
    BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));
    circularBuffer.pushBuffer(testBuffer);

    SECTION("Prediction narrows search range")
    {
        // Find first mark
        juce::int64 searchEnd = bufferSize - 1000;
        juce::int64 searchStart = searchEnd - static_cast<juce::int64>(expectedPeriod);
        juce::Range<juce::int64> searchRange(searchStart, searchEnd);

        marker.doPitchMarking(circularBuffer, searchRange, expectedPeriod, bufferSize, false);

        juce::int64 prediction = marker.getPredictedNextMark();
        REQUIRE(prediction > 0);

        // When using prediction, the actual search will be narrowed to +/- 25% around prediction
        // We provide a wide search range but prediction should narrow it
        juce::int64 wideSearchStart = prediction - 1000;
        juce::int64 wideSearchEnd = prediction + 1000;
        juce::Range<juce::int64> wideRange(wideSearchStart, wideSearchEnd);

        juce::int64 secondMark = marker.doPitchMarking(circularBuffer, wideRange, expectedPeriod, bufferSize, true);

        // Second mark should be close to prediction
        juce::int64 diff = std::abs(secondMark - prediction);
        REQUIRE(diff < expectedPeriod / 2); // Within half a period

        // Both marks should be stored
        REQUIRE(marker.getNumStoredMarks() == 2);
    }

    SECTION("Without prediction, uses full search range")
    {
        marker.reset();

        juce::int64 searchEnd = bufferSize;
        juce::int64 searchStart = searchEnd - static_cast<juce::int64>(expectedPeriod);
        juce::Range<juce::int64> searchRange(searchStart, searchEnd);

        juce::int64 foundMark = marker.doPitchMarking(circularBuffer, searchRange, expectedPeriod, bufferSize, false);

        // Should search full range
        REQUIRE(foundMark >= searchStart);
        REQUIRE(foundMark < searchEnd);

        // Mark should be stored
        REQUIRE(marker.getNumStoredMarks() == 1);
    }
}

//=======================================
TEST_CASE("PitchMarker - Reset Clears State", "[PitchMarker]")
{
    const double sampleRate = 44100.0;
    const float frequency = 440.0f;
    const float expectedPeriod = static_cast<float>(sampleRate / frequency);

    PitchMarker marker;
    marker.prepare(sampleRate, 2048);

    CircularBuffer circularBuffer;

    const int bufferSize = 4096;
    circularBuffer.setSize(1, bufferSize);

    juce::AudioBuffer<float> testBuffer(1, bufferSize);
    BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));
    circularBuffer.pushBuffer(testBuffer);

    // Find a mark
    juce::int64 searchEnd = bufferSize;
    juce::int64 searchStart = searchEnd - static_cast<juce::int64>(expectedPeriod);
    juce::Range<juce::int64> searchRange(searchStart, searchEnd);

    marker.doPitchMarking(circularBuffer, searchRange, expectedPeriod, bufferSize, false);

    REQUIRE(marker.getLastMark() != -1);
    REQUIRE(marker.getPredictedNextMark() != -1);
    REQUIRE(marker.getNumStoredMarks() == 1);

    // Reset should clear state
    marker.reset();
    REQUIRE(marker.getLastMark() == -1);
    REQUIRE(marker.getPredictedNextMark() == -1);
    REQUIRE(marker.getNumStoredMarks() == 0);
}

//=======================================
TEST_CASE("PitchMarker - Different Frequencies", "[PitchMarker]")
{
    const double sampleRate = 44100.0;
    PitchMarker marker;
    marker.prepare(sampleRate, 2048);

    CircularBuffer circularBuffer;

    const int bufferSize = 4096;
    circularBuffer.setSize(1, bufferSize);

    SECTION("Low frequency (100 Hz)")
    {
        const float frequency = 100.0f;
        const float expectedPeriod = static_cast<float>(sampleRate / frequency);

        juce::AudioBuffer<float> testBuffer(1, bufferSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));
        circularBuffer.pushBuffer(testBuffer);

        juce::int64 searchEnd = bufferSize;
        juce::int64 searchStart = searchEnd - static_cast<juce::int64>(expectedPeriod);
        juce::Range<juce::int64> searchRange(searchStart, searchEnd);

        juce::int64 foundMark = marker.doPitchMarking(circularBuffer, searchRange, expectedPeriod, bufferSize, false);

        REQUIRE(foundMark >= searchStart);
        REQUIRE(foundMark < searchEnd);
        REQUIRE(marker.getNumStoredMarks() == 1);
    }

    SECTION("High frequency (1000 Hz)")
    {
        marker.reset();
        circularBuffer.getBuffer().clear();

        const float frequency = 1000.0f;
        const float expectedPeriod = static_cast<float>(sampleRate / frequency);

        juce::AudioBuffer<float> testBuffer(1, bufferSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(expectedPeriod));
        circularBuffer.pushBuffer(testBuffer);

        juce::int64 searchEnd = bufferSize;
        juce::int64 searchStart = searchEnd - static_cast<juce::int64>(expectedPeriod);
        juce::Range<juce::int64> searchRange(searchStart, searchEnd);

        juce::int64 foundMark = marker.doPitchMarking(circularBuffer, searchRange, expectedPeriod, bufferSize, false);

        REQUIRE(foundMark >= searchStart);
        REQUIRE(foundMark < searchEnd);
        REQUIRE(marker.getNumStoredMarks() == 1);
    }
}
