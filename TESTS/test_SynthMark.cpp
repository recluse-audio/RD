/**
 * test_SynthMark.cpp
 * Tests for SynthMark class
 */

#include <catch2/catch_test_macros.hpp>
#include "../SOURCE/PITCH/SynthMark.h"
#include "../SOURCE/PITCH/PitchMark.h"

//=======================================
TEST_CASE("SynthMark - Basic Construction", "[SynthMark]")
{
    SECTION("Default constructor creates invalid mark")
    {
        SynthMark mark;
        REQUIRE(!mark.isValid());
        REQUIRE(mark.pitchMark == -1);
        REQUIRE(mark.synthMark == -1);
        REQUIRE(mark.pitchRangeStart == -1);
        REQUIRE(mark.pitchRangeEnd == -1);
        REQUIRE(mark.synthRangeStart == -1);
        REQUIRE(mark.synthRangeEnd == -1);
    }

    SECTION("Constructor with explicit values")
    {
        const juce::int64 pitchMarkPos = 1000;
        const juce::int64 pitchStart = 900;
        const juce::int64 pitchEnd = 1099;
        const juce::int64 synthMarkPos = 5000;
        const float outputPeriod = 120.0f;

        SynthMark mark(pitchMarkPos, pitchStart, pitchEnd, synthMarkPos, outputPeriod);

        REQUIRE(mark.isValid());

        // Pitch mark data
        REQUIRE(mark.pitchMark == pitchMarkPos);
        REQUIRE(mark.pitchRangeStart == pitchStart);
        REQUIRE(mark.pitchRangeEnd == pitchEnd);
        REQUIRE(mark.getPitchRangeLength() == 200);

        // Synth mark data
        REQUIRE(mark.synthMark == synthMarkPos);
        REQUIRE(mark.synthRangeStart == synthMarkPos - 120);  // synthMark - outputPeriod
        REQUIRE(mark.synthRangeEnd == synthMarkPos + 120 - 1);  // synthMark + outputPeriod - 1
        REQUIRE(mark.getSynthRangeLength() == 240);  // 2 * outputPeriod
    }
}

//=======================================
TEST_CASE("SynthMark - Construction from PitchMark", "[SynthMark]")
{
    SECTION("Copies pitch mark data correctly")
    {
        const juce::int64 pitchMarkPos = 2000;
        const float inputPeriod = 100.0f;
        PitchMark pitchMark(pitchMarkPos, inputPeriod);

        const juce::int64 synthMarkPos = 10000;
        const float outputPeriod = 150.0f;
        SynthMark synthMark(pitchMark, synthMarkPos, outputPeriod);

        REQUIRE(synthMark.isValid());

        // Should copy pitch mark data
        REQUIRE(synthMark.pitchMark == pitchMark.mark);
        REQUIRE(synthMark.pitchRangeStart == pitchMark.rangeStart);
        REQUIRE(synthMark.pitchRangeEnd == pitchMark.rangeEnd);

        // Should have its own synth mark data with output period
        REQUIRE(synthMark.synthMark == synthMarkPos);
        REQUIRE(synthMark.synthRangeStart == synthMarkPos - 150);
        REQUIRE(synthMark.synthRangeEnd == synthMarkPos + 150 - 1);
    }

    SECTION("Synth mark is independent of pitch mark changes")
    {
        // Create pitch mark
        PitchMark pitchMark(1000, 100.0f);

        // Create synth mark from it
        SynthMark synthMark(pitchMark, 5000, 100.0f);

        // Store original values
        juce::int64 originalPitchMark = synthMark.pitchMark;
        juce::int64 originalPitchStart = synthMark.pitchRangeStart;
        juce::int64 originalPitchEnd = synthMark.pitchRangeEnd;

        // Modify the original pitch mark (in real use, this could be reset/deleted)
        pitchMark = PitchMark(9999, 200.0f);

        // Synth mark should still have the original copied values
        REQUIRE(synthMark.pitchMark == originalPitchMark);
        REQUIRE(synthMark.pitchRangeStart == originalPitchStart);
        REQUIRE(synthMark.pitchRangeEnd == originalPitchEnd);
    }
}

//=======================================
TEST_CASE("SynthMark - Range Methods", "[SynthMark]")
{
    const juce::int64 pitchMarkPos = 1000;
    const float inputPeriod = 100.0f;
    PitchMark pitchMark(pitchMarkPos, inputPeriod);

    const juce::int64 synthMarkPos = 5000;
    const float outputPeriod = 150.0f;
    SynthMark synthMark(pitchMark, synthMarkPos, outputPeriod);

    SECTION("getPitchRange returns correct juce::Range")
    {
        juce::Range<juce::int64> range = synthMark.getPitchRange();
        REQUIRE(range.getStart() == 900);   // pitchMark - inputPeriod
        REQUIRE(range.getEnd() == 1100);    // pitchMark + inputPeriod (exclusive end)
        REQUIRE(range.getLength() == 200);
    }

    SECTION("getSynthRange returns correct juce::Range")
    {
        juce::Range<juce::int64> range = synthMark.getSynthRange();
        REQUIRE(range.getStart() == 4850);  // synthMark - outputPeriod
        REQUIRE(range.getEnd() == 5150);    // synthMark + outputPeriod (exclusive end)
        REQUIRE(range.getLength() == 300);
    }

    SECTION("Range lengths match expected periods")
    {
        REQUIRE(synthMark.getPitchRangeLength() == static_cast<juce::int64>(inputPeriod * 2));
        REQUIRE(synthMark.getSynthRangeLength() == static_cast<juce::int64>(outputPeriod * 2));
    }
}

//=======================================
TEST_CASE("SynthMark - Different Input and Output Periods", "[SynthMark]")
{
    SECTION("Output period larger than input (time stretching)")
    {
        const juce::int64 pitchMarkPos = 1000;
        const float inputPeriod = 100.0f;
        PitchMark pitchMark(pitchMarkPos, inputPeriod);

        const juce::int64 synthMarkPos = 5000;
        const float outputPeriod = 200.0f;  // 2x slower
        SynthMark synthMark(pitchMark, synthMarkPos, outputPeriod);

        REQUIRE(synthMark.getPitchRangeLength() == 200);  // Input: 2 * 100
        REQUIRE(synthMark.getSynthRangeLength() == 400);  // Output: 2 * 200
    }

    SECTION("Output period smaller than input (time compression)")
    {
        const juce::int64 pitchMarkPos = 1000;
        const float inputPeriod = 100.0f;
        PitchMark pitchMark(pitchMarkPos, inputPeriod);

        const juce::int64 synthMarkPos = 5000;
        const float outputPeriod = 50.0f;  // 2x faster
        SynthMark synthMark(pitchMark, synthMarkPos, outputPeriod);

        REQUIRE(synthMark.getPitchRangeLength() == 200);  // Input: 2 * 100
        REQUIRE(synthMark.getSynthRangeLength() == 100);  // Output: 2 * 50
    }
}
