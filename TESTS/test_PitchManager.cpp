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
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

//=======================================
// Helper function to validate synth marks against golden reference CSV
//=======================================
void validateSynthMarksAgainstGolden(const std::vector<SynthMark>& synthMarks, float shiftRatio)
{
    // Get path relative to this source file
    std::string goldenPath = __FILE__;
    size_t lastSlash = goldenPath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        goldenPath = goldenPath.substr(0, lastSlash + 1);
    }
    goldenPath += "GOLDEN/PITCH/GOLDEN_Sine_Wave_PitchMarks_4096range_256period_64peak.csv";

    std::ifstream file(goldenPath);
    INFO("Trying to open: " << goldenPath);
    REQUIRE(file.is_open());

    std::string line;
    bool foundShiftRatio = false;
    bool inSynthMarksSection = false;

    // Convert shift ratio to string for comparison (e.g., "1.0")
    std::ostringstream ratioStream;
    ratioStream << shiftRatio;
    std::string targetRatio = ratioStream.str();

    // Read through file to find the correct synth marks section
    while (std::getline(file, line))
    {
        // Skip empty lines
        if (line.empty())
            continue;

        // Check if this is a synth marks section header
        if (line.find("# SYNTH MARKS") != std::string::npos)
        {
            // If we already found our section, a new section header means we're done
            if (foundShiftRatio)
                break;

            // Check if this section matches our shift ratio
            if (line.find("Shift Ratio " + targetRatio) != std::string::npos ||
                line.find("Shift Ratio: " + targetRatio) != std::string::npos)
            {
                foundShiftRatio = true;
                inSynthMarksSection = true;

                // Read column headers
                std::getline(file, line);
                continue;
            }
        }

        // If we found our section and we're reading data lines
        if (foundShiftRatio && inSynthMarksSection)
        {
            // Stop if we hit another comment (new section)
            if (line[0] == '#')
                break;

            // Parse CSV line
            std::istringstream ss(line);
            std::string token;
            std::vector<std::string> tokens;

            while (std::getline(ss, token, ','))
            {
                tokens.push_back(token);
            }

            // Expecting: SynthMarkIndex, SynthRangeStart, SynthMarkPosition, SynthRangeEnd, SourcePitchMarkIndex
            if (tokens.size() >= 5)
            {
                int index = std::stoi(tokens[0]);
                juce::int64 expectedRangeStart = std::stoll(tokens[1]);
                juce::int64 expectedPosition = std::stoll(tokens[2]);
                juce::int64 expectedRangeEnd = std::stoll(tokens[3]);
                int expectedSourcePMIndex = std::stoi(tokens[4]);

                REQUIRE(index < static_cast<int>(synthMarks.size()));

                const auto& mark = synthMarks[index];

                INFO("Shift ratio: " << shiftRatio);
                INFO("Synth mark index: " << index);
                INFO("Actual synthMark: " << mark.synthMark << "  Expected: " << expectedPosition);
                REQUIRE(mark.synthMark == expectedPosition);
                REQUIRE(mark.synthRangeStart == expectedRangeStart);
                REQUIRE(mark.synthRangeEnd == expectedRangeEnd);

                // Note: We don't directly have source pitch mark index in SynthMark,
                // but we can verify the pitch mark position matches what we expect
                // by checking the pitchMark field
            }
        }
    }

    REQUIRE(foundShiftRatio);
}

//=======================================
TEST_CASE("PitchManager - Synth Mark Positions At Varied Shift Ratios", "[PitchManager][SynthMarker]")
{
    // Use fabricated pitch marks at evenly-spaced, known positions.
    // PitchMark(position, period) sets rangeStart = pos - period, rangeEnd = pos + period - 1.
    // First pitch mark centered at 64, then 8 marks spaced by period (256).
    const float knownPeriod = 256.0f;
    std::vector<PitchMark> pitchMarks;
    for (int i = 0; i < 8; ++i)
        pitchMarks.emplace_back(static_cast<juce::int64>(64 + i * knownPeriod), knownPeriod);

    // shiftedPeriod = knownPeriod / shiftRatio
    //   ratio > 1  →  shorter output period  (pitch up)
    //   ratio < 1  →  longer  output period  (pitch down)
    //
    // SynthMarker places the first synth mark at the first pitch mark position,
    // then each subsequent mark at +shiftedPeriod.
    // All tests use range [0, 2047] and check:
    //   1. Expected number of synth marks in range
    //   2. First synth mark == first pitch mark position
    //   3. Consecutive marks are spaced by shiftedPeriod (within 1 sample)

    const juce::Range<juce::int64> testRange(0, 2047);

    SECTION("Ratio 0.5  — pitch down octave,  shiftedPeriod=512")
    {
        PitchManager manager;
        manager.prepare(44100.0);

        const float shiftRatio = 0.5f;
        const float shiftedPeriod = knownPeriod / shiftRatio;
        const int expectedNumMarks = 4;

        manager.getSynthMarker().generateSynthMarks(pitchMarks, shiftedPeriod, testRange);
        const auto& marks = manager.getSynthMarks();

        REQUIRE(static_cast<int>(marks.size()) == expectedNumMarks);
        REQUIRE(marks[0].isValid());
        REQUIRE(marks[0].synthMark == pitchMarks[0].mark);

        for (int i = 1; i < expectedNumMarks; ++i)
        {
            REQUIRE(marks[i].isValid());
            const float spacing = static_cast<float>(marks[i].synthMark - marks[i - 1].synthMark);
            REQUIRE_THAT(spacing, Catch::Matchers::WithinAbs(shiftedPeriod, 1.0f));
        }

        // Validate against golden reference CSV
        validateSynthMarksAgainstGolden(marks, shiftRatio);
    }

    SECTION("Ratio 0.75 — pitch down,          shiftedPeriod≈341")
    {
        PitchManager manager;
        manager.prepare(44100.0);

        const float shiftRatio = 0.75f;
        const float shiftedPeriod = knownPeriod / shiftRatio;
        const int expectedNumMarks = 6;

        manager.getSynthMarker().generateSynthMarks(pitchMarks, shiftedPeriod, testRange);
        const auto& marks = manager.getSynthMarks();

        REQUIRE(static_cast<int>(marks.size()) == expectedNumMarks);
        REQUIRE(marks[0].isValid());
        REQUIRE(marks[0].synthMark == pitchMarks[0].mark);

        for (int i = 1; i < expectedNumMarks; ++i)
        {
            REQUIRE(marks[i].isValid());
            const float spacing = static_cast<float>(marks[i].synthMark - marks[i - 1].synthMark);
            REQUIRE_THAT(spacing, Catch::Matchers::WithinAbs(shiftedPeriod, 1.0f));
        }

        // Validate against golden reference CSV
        validateSynthMarksAgainstGolden(marks, shiftRatio);
    }

    SECTION("Ratio 0.9  — slight pitch down,   shiftedPeriod≈284")
    {
        PitchManager manager;
        manager.prepare(44100.0);

        const float shiftRatio = 0.9f;
        const float shiftedPeriod = knownPeriod / shiftRatio;
        const int expectedNumMarks = 7;

        manager.getSynthMarker().generateSynthMarks(pitchMarks, shiftedPeriod, testRange);
        const auto& marks = manager.getSynthMarks();

        REQUIRE(static_cast<int>(marks.size()) == expectedNumMarks);
        REQUIRE(marks[0].isValid());
        REQUIRE(marks[0].synthMark == pitchMarks[0].mark);

        for (int i = 1; i < expectedNumMarks; ++i)
        {
            REQUIRE(marks[i].isValid());
            const float spacing = static_cast<float>(marks[i].synthMark - marks[i - 1].synthMark);
            REQUIRE_THAT(spacing, Catch::Matchers::WithinAbs(shiftedPeriod, 1.0f));
        }

        // Validate against golden reference CSV
        validateSynthMarksAgainstGolden(marks, shiftRatio);
    }

    SECTION("Ratio 1.0  — no shift, shiftedPeriod=256")
    {
        // Diagram: Pitch mark ranges vs synth mark positions (period = 256 samples)
        //

        //                                   P1       P2       P3       P4       P5       P6       P7       P8
        //                          (-192)   (64)     (320)    (576)    (832)    (1088)   (1344)   (1600)   (1856)
        //                          |        |        |        |        |        |        |        |        |         |
        //
        // PM1 Range [-192,319]:    [========|========]
        // PM2 Range [64,575]:               [========|========]
        // PM3 Range [320,831]:                       [========|========]
        // PM4 Range [576,1087]:                               [========|========]
        // PM5 Range [832,1343]:                                        [========|========]
        // PM6 Range [1088,1599]:                                                [========|========]
        // PM7 Range [1344,1855]:                                                         [========|========]
        // PM8 Range [1600,2111]:                                                                  [========|========]
        //
        //                                   S1       S2       S3       S4       S5       S6       S7       S8
        //                          (-192)   (64)     (320)    (576)    (832)    (1088)   (1344)   (1600)   (1856)
        //                          |        |        |        |        |        |        |        |        |         |
        //
        // SM1 Range [-192,319]:    [========|========]
        // SM2 Range [64,575]:               [========|========]
        // SM3 Range [320,831]:                       [========|========]
        // SM4 Range [576,1087]:                               [========|========]
        // SM5 Range [832,1343]:                                        [========|========]
        // SM6 Range [1088,1599]:                                                [========|========]
        // SM7 Range [1344,1855]:                                                         [========|========]
        // SM8 Range [1600,2111]:                                                                  [========|========]
        //
        // Test Range [0,2047]:          [================================================================================================]
        //
        // Expected: 8 synth marks at positions 64, 320, 576, 832, 1088, 1344, 1600, 1856
        // Each synth mark reads from the pitch mark whose range [mark, rangeEnd] contains its position

        PitchManager manager;
        manager.prepare(44100.0);

        const float shiftRatio = 1.0f;
        const float shiftedPeriod = knownPeriod / shiftRatio;
        const int expectedNumMarks = 8;

        manager.getSynthMarker().generateSynthMarks(pitchMarks, shiftedPeriod, testRange);
        const auto& marks = manager.getSynthMarks();

        REQUIRE(static_cast<int>(marks.size()) == expectedNumMarks);

        // Verify exact synth mark positions and ranges from diagram
        REQUIRE(marks[0].synthMark == 64);
        REQUIRE(marks[0].pitchMark == 64);
        REQUIRE(marks[0].pitchRangeStart == -192);
        REQUIRE(marks[0].pitchRangeEnd == 319);

        REQUIRE(marks[1].synthMark == 320);
        REQUIRE(marks[1].pitchMark == 320);
        REQUIRE(marks[1].pitchRangeStart == 64);
        REQUIRE(marks[1].pitchRangeEnd == 575);

        REQUIRE(marks[2].synthMark == 576);
        REQUIRE(marks[2].pitchMark == 576);
        REQUIRE(marks[2].pitchRangeStart == 320);
        REQUIRE(marks[2].pitchRangeEnd == 831);

        REQUIRE(marks[3].synthMark == 832);
        REQUIRE(marks[3].pitchMark == 832);
        REQUIRE(marks[3].pitchRangeStart == 576);
        REQUIRE(marks[3].pitchRangeEnd == 1087);

        REQUIRE(marks[4].synthMark == 1088);
        REQUIRE(marks[4].pitchMark == 1088);
        REQUIRE(marks[4].pitchRangeStart == 832);
        REQUIRE(marks[4].pitchRangeEnd == 1343);

        REQUIRE(marks[5].synthMark == 1344);
        REQUIRE(marks[5].pitchMark == 1344);
        REQUIRE(marks[5].pitchRangeStart == 1088);
        REQUIRE(marks[5].pitchRangeEnd == 1599);

        REQUIRE(marks[6].synthMark == 1600);
        REQUIRE(marks[6].pitchMark == 1600);
        REQUIRE(marks[6].pitchRangeStart == 1344);
        REQUIRE(marks[6].pitchRangeEnd == 1855);

        REQUIRE(marks[7].synthMark == 1856);
        REQUIRE(marks[7].pitchMark == 1856);
        REQUIRE(marks[7].pitchRangeStart == 1600);
        REQUIRE(marks[7].pitchRangeEnd == 2111);

        // Verify synth ranges match the diagram (SM ranges same as PM ranges for ratio 1.0)
        REQUIRE(marks[0].synthRangeStart == -192);
        REQUIRE(marks[0].synthRangeEnd == 319);

        REQUIRE(marks[1].synthRangeStart == 64);
        REQUIRE(marks[1].synthRangeEnd == 575);

        REQUIRE(marks[2].synthRangeStart == 320);
        REQUIRE(marks[2].synthRangeEnd == 831);

        REQUIRE(marks[3].synthRangeStart == 576);
        REQUIRE(marks[3].synthRangeEnd == 1087);

        REQUIRE(marks[4].synthRangeStart == 832);
        REQUIRE(marks[4].synthRangeEnd == 1343);

        REQUIRE(marks[5].synthRangeStart == 1088);
        REQUIRE(marks[5].synthRangeEnd == 1599);

        REQUIRE(marks[6].synthRangeStart == 1344);
        REQUIRE(marks[6].synthRangeEnd == 1855);

        REQUIRE(marks[7].synthRangeStart == 1600);
        REQUIRE(marks[7].synthRangeEnd == 2111);

        // Validate against golden reference CSV
        validateSynthMarksAgainstGolden(marks, shiftRatio);
    }

    SECTION("Ratio 1.1  — slight pitch up,     shiftedPeriod≈233")
    {
        PitchManager manager;
        manager.prepare(44100.0);

        const float shiftRatio = 1.1f;
        const float shiftedPeriod = knownPeriod / shiftRatio;
        const int expectedNumMarks = 9;

        manager.getSynthMarker().generateSynthMarks(pitchMarks, shiftedPeriod, testRange);
        const auto& marks = manager.getSynthMarks();

        REQUIRE(static_cast<int>(marks.size()) == expectedNumMarks);
        REQUIRE(marks[0].isValid());
        REQUIRE(marks[0].synthMark == pitchMarks[0].mark);

        for (int i = 1; i < expectedNumMarks; ++i)
        {
            REQUIRE(marks[i].isValid());
            const float spacing = static_cast<float>(marks[i].synthMark - marks[i - 1].synthMark);
            REQUIRE_THAT(spacing, Catch::Matchers::WithinAbs(shiftedPeriod, 1.0f));
        }

        // Validate against golden reference CSV
        validateSynthMarksAgainstGolden(marks, shiftRatio);
    }

    SECTION("Ratio 1.25 — pitch up,            shiftedPeriod≈205")
    {
        PitchManager manager;
        manager.prepare(44100.0);

        const float shiftRatio = 1.25f;
        const float shiftedPeriod = knownPeriod / shiftRatio;
        const int expectedNumMarks = 10;

        manager.getSynthMarker().generateSynthMarks(pitchMarks, shiftedPeriod, testRange);
        const auto& marks = manager.getSynthMarks();

        REQUIRE(static_cast<int>(marks.size()) == expectedNumMarks);
        REQUIRE(marks[0].isValid());
        REQUIRE(marks[0].synthMark == pitchMarks[0].mark);

        for (int i = 1; i < expectedNumMarks; ++i)
        {
            REQUIRE(marks[i].isValid());
            const float spacing = static_cast<float>(marks[i].synthMark - marks[i - 1].synthMark);
            REQUIRE_THAT(spacing, Catch::Matchers::WithinAbs(shiftedPeriod, 1.0f));
        }

        // Validate against golden reference CSV
        validateSynthMarksAgainstGolden(marks, shiftRatio);
    }

    SECTION("Ratio 1.5  — pitch up,            shiftedPeriod≈171")
    {
        PitchManager manager;
        manager.prepare(44100.0);

        const float shiftRatio = 1.5f;
        const float shiftedPeriod = knownPeriod / shiftRatio;
        const int expectedNumMarks = 12;

        manager.getSynthMarker().generateSynthMarks(pitchMarks, shiftedPeriod, testRange);
        const auto& marks = manager.getSynthMarks();

        REQUIRE(static_cast<int>(marks.size()) == expectedNumMarks);
        REQUIRE(marks[0].isValid());
        REQUIRE(marks[0].synthMark == pitchMarks[0].mark);

        for (int i = 1; i < expectedNumMarks; ++i)
        {
            REQUIRE(marks[i].isValid());
            const float spacing = static_cast<float>(marks[i].synthMark - marks[i - 1].synthMark);
            REQUIRE_THAT(spacing, Catch::Matchers::WithinAbs(shiftedPeriod, 1.0f));
        }
    }

    SECTION("Ratio 2.0  — pitch up octave,     shiftedPeriod=128")
    {
        PitchManager manager;
        manager.prepare(44100.0);

        const float shiftRatio = 2.0f;
        const float shiftedPeriod = knownPeriod / shiftRatio;
        const int expectedNumMarks = 16;

        manager.getSynthMarker().generateSynthMarks(pitchMarks, shiftedPeriod, testRange);
        const auto& marks = manager.getSynthMarks();

        REQUIRE(static_cast<int>(marks.size()) == expectedNumMarks);
        REQUIRE(marks[0].isValid());
        REQUIRE(marks[0].synthMark == pitchMarks[0].mark);

        for (int i = 1; i < expectedNumMarks; ++i)
        {
            REQUIRE(marks[i].isValid());
            const float spacing = static_cast<float>(marks[i].synthMark - marks[i - 1].synthMark);
            REQUIRE_THAT(spacing, Catch::Matchers::WithinAbs(shiftedPeriod, 1.0f));
        }

        // Validate against golden reference CSV
        validateSynthMarksAgainstGolden(marks, shiftRatio);
    }
}

//=======================================
TEST_CASE("PitchManager - Configuration Access", "[PitchManager]")
{
    PitchManager manager;
    manager.prepare(44100.0);

    SECTION("Can access pitch detector for configuration")
    {
        FFT_PitchDetector& detector = manager.getPitchDetector();
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
