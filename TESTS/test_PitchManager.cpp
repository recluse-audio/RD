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
TEST_CASE("PitchManager - Synth Mark Positions At Varied Shift Ratios", "[PitchManager][SynthMarker]")
{
    // Use fabricated pitch marks at evenly-spaced, known positions.
    // PitchMark(position, period) sets rangeStart = pos - period, rangeEnd = pos + period - 1.
    // 8 marks at multiples of 256 give plenty of source coverage for all tested ratios.
    const float knownPeriod = 256.0f;
    std::vector<PitchMark> pitchMarks;
    for (int i = 1; i <= 8; ++i)
        pitchMarks.emplace_back(static_cast<juce::int64>(i * knownPeriod), knownPeriod);

    // shiftedPeriod = knownPeriod / shiftRatio
    //   ratio > 1  →  shorter output period  (pitch up)
    //   ratio < 1  →  longer  output period  (pitch down)
    //
    // SynthMarker places the first synth mark at the first pitch mark position,
    // then each subsequent mark at +shiftedPeriod.
    // Checks:
    //   1. Exactly numSynthMarks valid marks are returned
    //   2. First synth mark == first pitch mark position
    //   3. Consecutive marks are spaced by shiftedPeriod (within 1 sample)
    auto checkSpacing = [&](float shiftRatio)
    {
        PitchManager manager;
        manager.prepare(44100.0);

        const float     shiftedPeriod = knownPeriod / shiftRatio;
        constexpr int   numSynthMarks = 4;

        manager.getSynthMarker().generateSynthMarks(pitchMarks, shiftedPeriod, numSynthMarks);
        const auto& marks = manager.getSynthMarks();

        REQUIRE(static_cast<int>(marks.size()) == numSynthMarks);

        // First mark aligns with first pitch mark
        INFO("ratio=" << shiftRatio << " shiftedPeriod=" << shiftedPeriod
             << " first synthMark=" << marks[0].synthMark
             << " first pitchMark=" << pitchMarks[0].mark);
        REQUIRE(marks[0].isValid());
        REQUIRE(marks[0].synthMark == pitchMarks[0].mark);

        // Each pair of consecutive marks is spaced by shiftedPeriod (within 1 sample)
        for (int i = 1; i < numSynthMarks; ++i)
        {
            REQUIRE(marks[i].isValid());
            const float spacing = static_cast<float>(marks[i].synthMark - marks[i - 1].synthMark);
            INFO("ratio=" << shiftRatio << " mark[" << i << "] spacing=" << spacing
                 << " expected=" << shiftedPeriod);
            REQUIRE_THAT(spacing, Catch::Matchers::WithinAbs(shiftedPeriod, 1.0f));
        }
    };

    SECTION("Ratio 0.5  — pitch down octave,  shiftedPeriod=512") { checkSpacing(0.5f); }
    SECTION("Ratio 0.75 — pitch down,          shiftedPeriod≈341") { checkSpacing(0.75f); }
    SECTION("Ratio 0.9  — slight pitch down,   shiftedPeriod≈284") { checkSpacing(0.9f); }
    SECTION("Ratio 1.0  — no shift,            shiftedPeriod=256") { checkSpacing(1.0f); }
    SECTION("Ratio 1.1  — slight pitch up,     shiftedPeriod≈233") { checkSpacing(1.1f); }
    SECTION("Ratio 1.25 — pitch up,            shiftedPeriod≈205") { checkSpacing(1.25f); }
    SECTION("Ratio 1.5  — pitch up,            shiftedPeriod≈171") { checkSpacing(1.5f); }
    SECTION("Ratio 2.0  — pitch up octave,     shiftedPeriod=128") { checkSpacing(2.0f); }
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
