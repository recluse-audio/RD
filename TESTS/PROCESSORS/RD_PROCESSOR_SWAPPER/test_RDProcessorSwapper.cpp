#include <catch2/catch_test_macros.hpp>
#include "../../../SOURCE/PROCESSORS/RD_ProcessorSwapper.h"
#include "../../../SOURCE/BUFFER_FILLER/BufferFiller.h"
#include "../../TEST_UTILS/TestUtils.h"

TEST_CASE("RD_ProcessorSwapper active processor index get and set", "[RD_ProcessorSwapper]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;

    SECTION("Default active processor index is kGain")
    {
        REQUIRE(swapper.getActiveProcessorIndex() == RD_ProcessorSwapper::ProcessorIndex::kGain);
    }

    SECTION("Setting active processor to kGrainShifter is reflected by getter")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGrainShifter);
        REQUIRE(swapper.getActiveProcessorIndex() == RD_ProcessorSwapper::ProcessorIndex::kGrainShifter);
    }

    SECTION("Setting active processor back to kGain is reflected by getter")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGrainShifter);
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGain);
        REQUIRE(swapper.getActiveProcessorIndex() == RD_ProcessorSwapper::ProcessorIndex::kGain);
    }

    SECTION("getActiveProcessor returns a non-null pointer for kGain")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGain);
        REQUIRE(swapper.getActiveProcessor() != nullptr);
    }

    SECTION("getActiveProcessor returns a non-null pointer for kGrainShifter")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGrainShifter);
        REQUIRE(swapper.getActiveProcessor() != nullptr);
    }

    SECTION("getActiveProcessor returns the correct concrete type for kGain")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGain);
        REQUIRE(dynamic_cast<GainProcessor*> (swapper.getActiveProcessor()) != nullptr);
    }

    SECTION("getActiveProcessor returns the correct concrete type for kGrainShifter")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGrainShifter);
        REQUIRE(dynamic_cast<GrainShifterProcessor*> (swapper.getActiveProcessor()) != nullptr);
    }
}

TEST_CASE("RD_ProcessorSwapper fade and swap", "[RD_ProcessorSwapper]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;

    const double sampleRate = 44100.0;
    const int blockSize = 512;
    swapper.prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    SECTION("Fade state is kNoFade before any swap is triggered")
    {
        REQUIRE(swapper.getFadeState() == Fade::FadeState::kNoFade);
    }

}
