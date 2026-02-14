/**
 * test_SynthMarker.cpp
 * Tests for SynthMarker class
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../SOURCE/PITCH/SynthMarker.h"
#include "../SOURCE/PITCH/PitchMark.h"
#include "../SOURCE/PITCH/SynthMark.h"

//=======================================
TEST_CASE("SynthMarker - Basic Construction", "[SynthMarker]")
{
    SynthMarker marker;

    SECTION("Initial state")
    {
        REQUIRE(marker.getNumSynthMarks() == 0);
        REQUIRE(marker.getSynthMarks().empty());
    }

    SECTION("Reset")
    {
        marker.reset();
        REQUIRE(marker.getNumSynthMarks() == 0);
    }
}

//=======================================
TEST_CASE("SynthMarker - Generate Synth Marks - No Pitch Shift", "[SynthMarker]")
{
    SynthMarker marker;

    // Create pitch marks at positions 100, 200, 300 with period 100
    std::vector<PitchMark> pitchMarks;
    const float period = 100.0f;
    pitchMarks.push_back(PitchMark(100, period));   // Range [0, 199]
    pitchMarks.push_back(PitchMark(200, period));   // Range [100, 299]
    pitchMarks.push_back(PitchMark(300, period));   // Range [200, 399]

    SECTION("Generate synth marks with same period (no time stretching)")
    {
        const float shiftedPeriod = 100.0f;  // Same as input period
        const int numSynthMarks = 3;

        marker.generateSynthMarks(pitchMarks, shiftedPeriod, numSynthMarks);

        REQUIRE(marker.getNumSynthMarks() == 3);

        const auto& synthMarks = marker.getSynthMarks();

        // First synth mark should match first pitch mark
        REQUIRE(synthMarks[0].synthMark == 100);
        REQUIRE(synthMarks[0].pitchMark == 100);
        REQUIRE(synthMarks[0].pitchRangeStart == 0);
        REQUIRE(synthMarks[0].pitchRangeEnd == 199);

        // Second synth mark at 200 (100 + 100*1)
        REQUIRE(synthMarks[1].synthMark == 200);
        REQUIRE(synthMarks[1].pitchMark == 200);  // Uses pitch mark at 200
        REQUIRE(synthMarks[1].pitchRangeStart == 100);
        REQUIRE(synthMarks[1].pitchRangeEnd == 299);

        // Third synth mark at 300 (100 + 100*2)
        REQUIRE(synthMarks[2].synthMark == 300);
        REQUIRE(synthMarks[2].pitchMark == 300);  // Uses pitch mark at 300
        REQUIRE(synthMarks[2].pitchRangeStart == 200);
        REQUIRE(synthMarks[2].pitchRangeEnd == 399);
    }
}

//=======================================
TEST_CASE("SynthMarker - Pitch Mark Range Assignment Rule", "[SynthMarker]")
{
    SynthMarker marker;

    // Create pitch mark at 128 with period 128
    // Range will be [0, 255] with mark at 128
    std::vector<PitchMark> pitchMarks;
    pitchMarks.push_back(PitchMark(128, 128.0f));  // Range [0, 255]

    SECTION("Synth marks from 128-255 use the same pitch mark range")
    {
        const float shiftedPeriod = 50.0f;
        const int numSynthMarks = 3;  // Will be at 128, 178, 228

        marker.generateSynthMarks(pitchMarks, shiftedPeriod, numSynthMarks);

        REQUIRE(marker.getNumSynthMarks() == 3);

        const auto& synthMarks = marker.getSynthMarks();

        // Synth mark at 128 uses pitch mark [0, 255]
        REQUIRE(synthMarks[0].synthMark == 128);
        REQUIRE(synthMarks[0].pitchMark == 128);
        REQUIRE(synthMarks[0].pitchRangeStart == 0);
        REQUIRE(synthMarks[0].pitchRangeEnd == 255);

        // Synth mark at 178 (128 + 50) still uses same pitch mark
        REQUIRE(synthMarks[1].synthMark == 178);
        REQUIRE(synthMarks[1].pitchMark == 128);  // Still uses pitch mark at 128
        REQUIRE(synthMarks[1].pitchRangeStart == 0);
        REQUIRE(synthMarks[1].pitchRangeEnd == 255);

        // Synth mark at 228 (128 + 100) still uses same pitch mark
        REQUIRE(synthMarks[2].synthMark == 228);
        REQUIRE(synthMarks[2].pitchMark == 128);  // Still uses pitch mark at 128
        REQUIRE(synthMarks[2].pitchRangeStart == 0);
        REQUIRE(synthMarks[2].pitchRangeEnd == 255);
    }
}

//=======================================
TEST_CASE("SynthMarker - Time Stretching (Slower Output)", "[SynthMarker]")
{
    SynthMarker marker;

    // Create pitch marks at 100, 200 with period 100
    std::vector<PitchMark> pitchMarks;
    const float inputPeriod = 100.0f;
    pitchMarks.push_back(PitchMark(100, inputPeriod));   // Range [0, 199]
    pitchMarks.push_back(PitchMark(200, inputPeriod));   // Range [100, 299]

    SECTION("Generate synth marks with larger period (slower/stretched)")
    {
        const float shiftedPeriod = 150.0f;  // 1.5x slower
        const int numSynthMarks = 3;

        marker.generateSynthMarks(pitchMarks, shiftedPeriod, numSynthMarks);

        REQUIRE(marker.getNumSynthMarks() == 3);

        const auto& synthMarks = marker.getSynthMarks();

        // First synth mark at 100
        REQUIRE(synthMarks[0].synthMark == 100);
        REQUIRE(synthMarks[0].pitchMark == 100);
        REQUIRE(synthMarks[0].getSynthRangeLength() == 300);  // 2 * 150

        // Second synth mark at 250 (100 + 150)
        REQUIRE(synthMarks[1].synthMark == 250);
        // Position 250 is in range [200, 299] so uses pitch mark at 200
        REQUIRE(synthMarks[1].pitchMark == 200);
        REQUIRE(synthMarks[1].pitchRangeStart == 100);
        REQUIRE(synthMarks[1].pitchRangeEnd == 299);

        // Third synth mark at 400 (100 + 300)
        REQUIRE(synthMarks[2].synthMark == 400);
        // Position 400 is beyond all pitch marks, uses last pitch mark
        REQUIRE(synthMarks[2].pitchMark == 200);
    }
}

//=======================================
TEST_CASE("SynthMarker - Time Compression (Faster Output)", "[SynthMarker]")
{
    SynthMarker marker;

    // Create pitch marks at 100, 200, 300 with period 100
    std::vector<PitchMark> pitchMarks;
    const float inputPeriod = 100.0f;
    pitchMarks.push_back(PitchMark(100, inputPeriod));   // Range [0, 199]
    pitchMarks.push_back(PitchMark(200, inputPeriod));   // Range [100, 299]
    pitchMarks.push_back(PitchMark(300, inputPeriod));   // Range [200, 399]

    SECTION("Generate synth marks with smaller period (faster/compressed)")
    {
        const float shiftedPeriod = 50.0f;  // 2x faster
        const int numSynthMarks = 5;

        marker.generateSynthMarks(pitchMarks, shiftedPeriod, numSynthMarks);

        REQUIRE(marker.getNumSynthMarks() == 5);

        const auto& synthMarks = marker.getSynthMarks();

        // Synth marks will be at: 100, 150, 200, 250, 300
        REQUIRE(synthMarks[0].synthMark == 100);
        REQUIRE(synthMarks[1].synthMark == 150);
        REQUIRE(synthMarks[2].synthMark == 200);
        REQUIRE(synthMarks[3].synthMark == 250);
        REQUIRE(synthMarks[4].synthMark == 300);

        // Check pitch mark assignments based on [mark, rangeEnd] rule
        REQUIRE(synthMarks[0].pitchMark == 100);  // 100 in [100, 199]
        REQUIRE(synthMarks[1].pitchMark == 100);  // 150 in [100, 199]
        REQUIRE(synthMarks[2].pitchMark == 200);  // 200 in [200, 299]
        REQUIRE(synthMarks[3].pitchMark == 200);  // 250 in [200, 299]
        REQUIRE(synthMarks[4].pitchMark == 300);  // 300 in [300, 399]

        // All synth marks should have same output period
        for (const auto& sm : synthMarks)
        {
            REQUIRE(sm.getSynthRangeLength() == 100);  // 2 * 50
        }
    }
}

//=======================================
TEST_CASE("SynthMarker - Edge Cases", "[SynthMarker]")
{
    SynthMarker marker;

    SECTION("Empty pitch marks")
    {
        std::vector<PitchMark> pitchMarks;
        marker.generateSynthMarks(pitchMarks, 100.0f, 5);

        REQUIRE(marker.getNumSynthMarks() == 0);
    }

    SECTION("Zero synth marks requested")
    {
        std::vector<PitchMark> pitchMarks;
        pitchMarks.push_back(PitchMark(100, 100.0f));

        marker.generateSynthMarks(pitchMarks, 100.0f, 0);

        REQUIRE(marker.getNumSynthMarks() == 0);
    }

    SECTION("Invalid pitch marks")
    {
        std::vector<PitchMark> pitchMarks;
        pitchMarks.push_back(PitchMark());  // Invalid
        pitchMarks.push_back(PitchMark());  // Invalid

        marker.generateSynthMarks(pitchMarks, 100.0f, 3);

        REQUIRE(marker.getNumSynthMarks() == 0);
    }

    SECTION("Mix of valid and invalid pitch marks")
    {
        std::vector<PitchMark> pitchMarks;
        pitchMarks.push_back(PitchMark());            // Invalid
        pitchMarks.push_back(PitchMark(100, 100.0f)); // Valid
        pitchMarks.push_back(PitchMark());            // Invalid

        marker.generateSynthMarks(pitchMarks, 100.0f, 2);

        REQUIRE(marker.getNumSynthMarks() == 2);

        const auto& synthMarks = marker.getSynthMarks();
        // Should use the only valid pitch mark
        REQUIRE(synthMarks[0].pitchMark == 100);
        REQUIRE(synthMarks[1].pitchMark == 100);
    }
}

//=======================================
TEST_CASE("SynthMarker - Reset Clears Synth Marks", "[SynthMarker]")
{
    SynthMarker marker;

    std::vector<PitchMark> pitchMarks;
    pitchMarks.push_back(PitchMark(100, 100.0f));

    marker.generateSynthMarks(pitchMarks, 100.0f, 3);
    REQUIRE(marker.getNumSynthMarks() == 3);

    marker.reset();
    REQUIRE(marker.getNumSynthMarks() == 0);
    REQUIRE(marker.getSynthMarks().empty());
}

//=======================================
TEST_CASE("SynthMarker - Get Marks In Range", "[SynthMarker]")
{
    SynthMarker marker;

    // Create pitch marks at positions 100, 200, 300 with period 100
    std::vector<PitchMark> pitchMarks;
    const float period = 100.0f;
    pitchMarks.push_back(PitchMark(100, period));   // Range [0, 199]
    pitchMarks.push_back(PitchMark(200, period));   // Range [100, 299]
    pitchMarks.push_back(PitchMark(300, period));   // Range [200, 399]

    // Generate synth marks with same period (no time stretching)
    // Synth marks will be at: 100, 200, 300
    const float shiftedPeriod = 100.0f;
    const int numSynthMarks = 3;

    marker.generateSynthMarks(pitchMarks, shiftedPeriod, numSynthMarks);
    REQUIRE(marker.getNumSynthMarks() == 3);

    SECTION("Get all marks in wide range")
    {
        auto marks = marker.getSynthMarksInRange(juce::Range<juce::int64>(0, 400));
        REQUIRE(marks.size() == 3);

        // Verify they are at expected positions
        REQUIRE(marks[0].synthMark == 100);
        REQUIRE(marks[1].synthMark == 200);
        REQUIRE(marks[2].synthMark == 300);
    }

    SECTION("Get marks in partial range")
    {
        // Range that includes first two marks (100, 200) but not third (300)
        auto marks = marker.getSynthMarksInRange(juce::Range<juce::int64>(50, 250));
        REQUIRE(marks.size() == 2);
        REQUIRE(marks[0].synthMark == 100);
        REQUIRE(marks[1].synthMark == 200);
    }

    SECTION("Get marks in narrow range around one mark")
    {
        // Range that only includes the mark at 200
        auto marks = marker.getSynthMarksInRange(juce::Range<juce::int64>(150, 250));
        REQUIRE(marks.size() == 1);
        REQUIRE(marks[0].synthMark == 200);
    }

    SECTION("Get no marks from empty range")
    {
        auto marks = marker.getSynthMarksInRange(juce::Range<juce::int64>(500, 600));
        REQUIRE(marks.size() == 0);
    }

    SECTION("Edge case - range boundary exactly on mark")
    {
        // Range from 100 to 200 (exclusive end)
        auto marks = marker.getSynthMarksInRange(juce::Range<juce::int64>(100, 200));
        REQUIRE(marks.size() == 1);  // Should include 100, exclude 200
        REQUIRE(marks[0].synthMark == 100);
    }
}

//=======================================
TEST_CASE("SynthMarker - Get Marks In Range With Time Stretching", "[SynthMarker]")
{
    SynthMarker marker;

    // Create pitch marks at 100, 200 with period 100
    std::vector<PitchMark> pitchMarks;
    const float period = 100.0f;
    pitchMarks.push_back(PitchMark(100, period));
    pitchMarks.push_back(PitchMark(200, period));

    // Generate synth marks with larger period (time stretching)
    // Synth marks will be at: 100, 250, 400
    const float shiftedPeriod = 150.0f;
    const int numSynthMarks = 3;

    marker.generateSynthMarks(pitchMarks, shiftedPeriod, numSynthMarks);
    REQUIRE(marker.getNumSynthMarks() == 3);

    SECTION("Get marks with stretched spacing")
    {
        // Range that includes first two synth marks
        auto marks = marker.getSynthMarksInRange(juce::Range<juce::int64>(50, 300));
        REQUIRE(marks.size() == 2);
        REQUIRE(marks[0].synthMark == 100);
        REQUIRE(marks[1].synthMark == 250);
    }

    SECTION("Verify returned marks contain correct pitch mark references")
    {
        auto marks = marker.getSynthMarksInRange(juce::Range<juce::int64>(0, 500));
        REQUIRE(marks.size() == 3);

        // First synth mark at 100 uses pitch mark at 100
        REQUIRE(marks[0].pitchMark == 100);

        // Second synth mark at 250 uses pitch mark at 200 (250 is in range [200, 299])
        REQUIRE(marks[1].pitchMark == 200);
    }
}

//=======================================
TEST_CASE("SynthMarker - Multiple Generations", "[SynthMarker]")
{
    SynthMarker marker;

    std::vector<PitchMark> pitchMarks1;
    pitchMarks1.push_back(PitchMark(100, 100.0f));

    // First generation
    marker.generateSynthMarks(pitchMarks1, 100.0f, 2);
    REQUIRE(marker.getNumSynthMarks() == 2);

    // Second generation (should replace first)
    std::vector<PitchMark> pitchMarks2;
    pitchMarks2.push_back(PitchMark(200, 100.0f));

    marker.generateSynthMarks(pitchMarks2, 50.0f, 5);
    REQUIRE(marker.getNumSynthMarks() == 5);

    const auto& synthMarks = marker.getSynthMarks();
    REQUIRE(synthMarks[0].synthMark == 200);  // Based on new pitch marks
}
