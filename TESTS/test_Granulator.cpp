/**
 * test_Granulator.cpp
 * Tests for Granulator class
 */

#include <catch2/catch_test_macros.hpp>
#include "../SOURCE/PROCESSORS/GRAIN/Granulator.h"
#include "../SOURCE/CircularBuffer.h"
#include "../SOURCE/PITCH/SynthMark.h"

TEST_CASE("Granulator - prepare()", "[Granulator]")
{
    CircularBuffer sourceBuffer;
    Granulator granulator(sourceBuffer);

    constexpr int maxGrains = 8;
    granulator.prepare(44100.0, 2, 512, maxGrains);

    REQUIRE(granulator.getNumGrains() == maxGrains);

    for (const auto& grain : granulator.getGrains())
    {
        REQUIRE_FALSE(grain.isValid());
    }
}

TEST_CASE("Granulator - generateGrains()", "[Granulator]")
{
    CircularBuffer sourceBuffer;
    constexpr juce::int64 lookahead = 512;
    Granulator granulator(sourceBuffer);
    granulator.prepare(44100.0, 2, lookahead, 8);

    // Mark 0: pitch[0,255], synth[0,255], writeRange [512, 767]
    // Mark 1: pitch[128,383], synth[128,383], writeRange [640, 895]
    // Mark 2: pitch[256,511], synth[256,511], writeRange [768, 1023]
    std::vector<SynthMark> synthMarks = {
        SynthMark(128, 0, 255, 128),
        SynthMark(256, 128, 383, 256),
        SynthMark(384, 256, 511, 384),
    };

    granulator.generateGrains(synthMarks);

    std::vector<const Grain*> validGrains;
    for (const auto& grain : granulator.getGrains())
    {
        if (grain.isValid())
            validGrains.push_back(&grain);
    }

    REQUIRE(validGrains.size() == synthMarks.size());

    for (size_t i = 0; i < synthMarks.size(); ++i)
    {
        REQUIRE(validGrains[i]->getWriteRangeStart() == synthMarks[i].synthRangeStart + lookahead);
        REQUIRE(validGrains[i]->getWriteRangeEnd()   == synthMarks[i].synthRangeEnd   + lookahead);
    }
}
