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

        SynthMark mark(pitchMarkPos, pitchStart, pitchEnd, synthMarkPos);

        REQUIRE(mark.isValid());

        // Pitch mark data
        REQUIRE(mark.pitchMark == pitchMarkPos);
        REQUIRE(mark.pitchRangeStart == pitchStart);
        REQUIRE(mark.pitchRangeEnd == pitchEnd);
        REQUIRE(mark.getPitchRangeLength() == 200);

        // Synth mark data - length matches pitch mark
        const juce::int64 pitchPeriod = 100;  // Derived from pitch mark position - pitch range start
        REQUIRE(mark.synthMark == synthMarkPos);
        REQUIRE(mark.synthRangeStart == synthMarkPos - pitchPeriod);
        REQUIRE(mark.synthRangeEnd == synthMarkPos + pitchPeriod - 1);
        REQUIRE(mark.getSynthRangeLength() == 200);  // Same as pitch range length
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
        SynthMark synthMark(pitchMark, synthMarkPos);

        REQUIRE(synthMark.isValid());

        // Should copy pitch mark data
        REQUIRE(synthMark.pitchMark == pitchMark.mark);
        REQUIRE(synthMark.pitchRangeStart == pitchMark.rangeStart);
        REQUIRE(synthMark.pitchRangeEnd == pitchMark.rangeEnd);

        // Synth mark length matches pitch mark length
        REQUIRE(synthMark.synthMark == synthMarkPos);
        REQUIRE(synthMark.synthRangeStart == synthMarkPos - 100);  // Uses pitch period
        REQUIRE(synthMark.synthRangeEnd == synthMarkPos + 100 - 1);
        REQUIRE(synthMark.getSynthRangeLength() == 200);  // Same as pitch range length (2 * inputPeriod)
    }

    SECTION("Synth mark is independent of pitch mark changes")
    {
        // Create pitch mark
        PitchMark pitchMark(1000, 100.0f);

        // Create synth mark from it
        SynthMark synthMark(pitchMark, 5000);

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
    SynthMark synthMark(pitchMark, synthMarkPos);

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
        REQUIRE(range.getStart() == 4900);  // synthMark - inputPeriod
        REQUIRE(range.getEnd() == 5100);    // synthMark + inputPeriod (exclusive end)
        REQUIRE(range.getLength() == 200);  // Same as pitch range
    }

    SECTION("Range lengths match - synth length equals pitch length")
    {
        REQUIRE(synthMark.getPitchRangeLength() == static_cast<juce::int64>(inputPeriod * 2));
        REQUIRE(synthMark.getSynthRangeLength() == static_cast<juce::int64>(inputPeriod * 2));  // Always matches pitch
    }
}

