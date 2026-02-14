/**
 * test_PitchManager.cpp
 * Tests for PitchManager class
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../SOURCE/PITCH/PitchManager.h"
#include "../SOURCE/CircularBuffer.h"
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/MarkWriter.h"
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
TEST_CASE("PitchManager - Get Marks In Range", "[PitchManager]")
{
    const double sampleRate = 44100.0;
    const int detectionWindowSize = 2048;
    const float frequency = 440.0f;
    const float period = sampleRate / frequency;

    PitchManager manager;
    manager.prepare(sampleRate, detectionWindowSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(1, 8192);

    SECTION("Get pitch marks in range after processing")
    {
        // Process audio to generate pitch marks
        juce::AudioBuffer<float> testBuffer(1, detectionWindowSize);
        BufferFiller::generateSineCycles(testBuffer, static_cast<int>(period));

        manager.process(testBuffer, circularBuffer);

        // Find some pitch marks
        const juce::int64 searchEnd = manager.getAbsoluteSampleCount();
        const juce::int64 searchStart = searchEnd - static_cast<juce::int64>(period);

        manager.findPitchMark(circularBuffer, juce::Range<juce::int64>(searchStart, searchEnd), false);

        // Get pitch marks in a range
        auto marks = manager.getPitchMarksInRange(juce::Range<juce::int64>(0, searchEnd + 1000));

        // Should have at least one pitch mark
        REQUIRE(marks.size() >= 1);
    }

    SECTION("Get synth marks in range")
    {
        // For now, synth marks array may be empty since we haven't generated them
        // But the method should work without crashing
        auto marks = manager.getSynthMarksInRange(juce::Range<juce::int64>(0, 1000));
        REQUIRE(marks.size() == 0);  // No synth marks generated yet
    }

    SECTION("Get all synth marks")
    {
        const auto& marks = manager.getSynthMarks();
        REQUIRE(marks.size() == 0);  // No synth marks generated yet
    }
}

//=======================================
TEST_CASE("PitchManager - Complete Workflow With Pitch and Synth Marks", "[PitchManager]")
{
    const double sampleRate = 44100.0;
    const int detectionWindowSize = 2048;
    const float period = 256.0f;  // Sine wave period

    PitchManager manager;
    manager.prepare(sampleRate, detectionWindowSize);

    CircularBuffer circularBuffer;
    circularBuffer.setSize(1, 8192);

    SECTION("Process sine wave, detect pitch, generate and verify marks")
    {
        // Process audio until we hit 2048 samples
        int totalSamplesProcessed = 0;
        const int targetSamples = 2048;
        const int bufferSize = 512;  // Process in chunks

        while (totalSamplesProcessed < targetSamples)
        {
            juce::AudioBuffer<float> testBuffer(1, bufferSize);

            // Generate sine wave with period 256
            BufferFiller::generateSineCycles(testBuffer, static_cast<int>(period));

            bool detected = manager.process(testBuffer, circularBuffer);
            totalSamplesProcessed += bufferSize;

            // Detection should occur when we fill the detection window (2048 samples)
            if (totalSamplesProcessed >= detectionWindowSize && detected)
            {
                // Check detected pitch
                float detectedPeriod = manager.getCurrentPeriod();
                REQUIRE(detectedPeriod > 0.0f);

                // Should be close to 256 (allow 5% error)
                float error = std::abs(detectedPeriod - period);
                REQUIRE(error < period * 0.05f);

                INFO("Detected period: " << detectedPeriod << ", Expected: " << period);
            }
        }

        REQUIRE(manager.getAbsoluteSampleCount() == targetSamples);

        const juce::int64 rangeStart = 0;
        const juce::int64 rangeEnd = 2047;

        // For a sine wave starting at phase 0 with period 256:
        // Peak occurs at period/4 = 64 samples
        // Expected peaks at: 64, 320, 576, 832, 1088, 1344, 1600, 1856
        const juce::int64 firstPeak = 64;
        std::vector<juce::int64> expectedPeaks;
        for (juce::int64 peak = firstPeak; peak <= rangeEnd; peak += static_cast<juce::int64>(period))
        {
            expectedPeaks.push_back(peak);
        }

        INFO("Expected " << expectedPeaks.size() << " peaks at positions starting from " << firstPeak);
        REQUIRE(expectedPeaks.size() == 8);  // Should have 8 peaks in 2048 samples

        // Find pitch marks at expected peak positions
        std::vector<PitchMark> pitchMarksForSynth;
        const juce::int64 searchRadius = static_cast<juce::int64>(period / 4);  // Search within ±period/4

        for (juce::int64 expectedPeak : expectedPeaks)
        {
            juce::int64 searchStart = expectedPeak - searchRadius;
            juce::int64 searchEnd = expectedPeak + searchRadius;
            juce::int64 foundMark = manager.findPitchMark(circularBuffer, juce::Range<juce::int64>(searchStart, searchEnd), false);

            REQUIRE(foundMark >= 0);  // Should find a mark

            // Verify found mark is close to expected peak (within 10% of period)
            juce::int64 error = std::abs(foundMark - expectedPeak);
            REQUIRE(error < static_cast<juce::int64>(period * 0.1f));

            INFO("Expected peak at " << expectedPeak << ", found mark at " << foundMark << ", error: " << error);

            pitchMarksForSynth.push_back(manager.getLastPitchMark());
        }

        // Get pitch marks in range [0, 2047]
        auto pitchMarks = manager.getPitchMarksInRange(juce::Range<juce::int64>(rangeStart, rangeEnd + 1));

        INFO("Found " << pitchMarks.size() << " pitch marks in range [0, 2047]");
        REQUIRE(pitchMarks.size() == expectedPeaks.size());  // Should match expected count

        // Verify each pitch mark is at an expected position
        for (size_t i = 0; i < pitchMarks.size(); ++i)
        {
            const auto& pm = pitchMarks[i];
            REQUIRE(pm.isValid());
            REQUIRE(pm.mark >= rangeStart);
            REQUIRE(pm.mark <= rangeEnd);

            // Verify this mark is close to one of the expected peaks
            bool nearExpectedPeak = false;
            for (juce::int64 expectedPeak : expectedPeaks)
            {
                if (std::abs(pm.mark - expectedPeak) < static_cast<juce::int64>(period * 0.1f))
                {
                    nearExpectedPeak = true;
                    break;
                }
            }
            REQUIRE(nearExpectedPeak);

            // Verify range is approximately 2 * period
            juce::int64 rangeLength = pm.getRangeLength();
            juce::int64 expectedRangeLength = static_cast<juce::int64>(manager.getCurrentPeriod() * 2);
            juce::int64 rangeLengthError = std::abs(rangeLength - expectedRangeLength);
            REQUIRE(rangeLengthError < static_cast<juce::int64>(period * 0.2f));  // Within 20%

            INFO("Pitch mark " << i << " at " << pm.mark
                 << " with range [" << pm.rangeStart << ", " << pm.rangeEnd << "], length: " << rangeLength);
        }

        // Generate synth marks with no pitch shifting (same period)
        const float shiftedPeriod = manager.getCurrentPeriod();  // No shifting - use detected period
        const int numSynthMarks = static_cast<int>(expectedPeaks.size());  // Generate one per pitch mark

        // Generate synth marks using PitchManager's SynthMarker
        manager.getSynthMarker().generateSynthMarks(pitchMarksForSynth, shiftedPeriod, numSynthMarks);

        // Get synth marks in range [0, 2047]
        auto synthMarks = manager.getSynthMarksInRange(juce::Range<juce::int64>(rangeStart, rangeEnd + 1));

        INFO("Found " << synthMarks.size() << " synth marks in range [0, 2047]");
        REQUIRE(synthMarks.size() == numSynthMarks);  // Should match requested count

        // With no pitch shifting (shiftedPeriod == detectedPeriod):
        // - First synth mark should match first pitch mark
        // - Subsequent synth marks spaced by shiftedPeriod
        // - Each synth mark should align with a pitch mark (or be very close)

        for (size_t i = 0; i < synthMarks.size(); ++i)
        {
            const auto& sm = synthMarks[i];
            REQUIRE(sm.isValid());
            REQUIRE(sm.synthMark >= rangeStart);
            REQUIRE(sm.synthMark <= rangeEnd);

            // Verify synth mark has valid pitch mark reference
            REQUIRE(sm.pitchMark >= 0);
            REQUIRE(sm.pitchRangeEnd > sm.pitchRangeStart);

            // Verify synth range matches expected size (2 * shiftedPeriod)
            juce::int64 synthRangeLength = sm.getSynthRangeLength();
            juce::int64 expectedSynthRangeLength = static_cast<juce::int64>(shiftedPeriod * 2);
            juce::int64 synthRangeLengthError = std::abs(synthRangeLength - expectedSynthRangeLength);
            REQUIRE(synthRangeLengthError < static_cast<juce::int64>(period * 0.2f));

            // With no shifting, synth mark should be close to an expected peak
            bool nearExpectedPeak = false;
            for (juce::int64 expectedPeak : expectedPeaks)
            {
                if (std::abs(sm.synthMark - expectedPeak) < static_cast<juce::int64>(period * 0.1f))
                {
                    nearExpectedPeak = true;
                    break;
                }
            }
            REQUIRE(nearExpectedPeak);

            // Verify synth mark references a pitch mark that's also near an expected peak
            bool pitchMarkNearExpectedPeak = false;
            for (juce::int64 expectedPeak : expectedPeaks)
            {
                if (std::abs(sm.pitchMark - expectedPeak) < static_cast<juce::int64>(period * 0.1f))
                {
                    pitchMarkNearExpectedPeak = true;
                    break;
                }
            }
            REQUIRE(pitchMarkNearExpectedPeak);

            INFO("Synth mark " << i << " at " << sm.synthMark
                 << " references pitch mark at " << sm.pitchMark
                 << " with pitch range [" << sm.pitchRangeStart << ", " << sm.pitchRangeEnd << "]"
                 << " and synth range [" << sm.synthRangeStart << ", " << sm.synthRangeEnd << "]");
        }

        // Write marks to CSV for analysis
        juce::String outputDir = juce::File::getCurrentWorkingDirectory()
            .getChildFile("SUBMODULES/RD/TESTS/OUTPUT")
            .getFullPathName();

        bool writeSuccess = MarkWriter::writeMarksToCSV(pitchMarks, synthMarks, outputDir);
        REQUIRE(writeSuccess);

        INFO("Marks written to OUTPUT/MARKS/MARKS_<timestamp>/marks_<timestamp>.csv");
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
