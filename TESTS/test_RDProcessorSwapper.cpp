#include <catch2/catch_test_macros.hpp>
#include "../SOURCE/PROCESSORS/RD_ProcessorSwapper.h"
#include "../SOURCE/BufferFiller.h"
#include "TEST_UTILS/TestUtils.h"

TEST_CASE("RD_ProcessorSwapper active processor index get and set", "[RD_ProcessorSwapper]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;

    SECTION("Default active processor index is kGain")
    {
        REQUIRE(swapper.getActiveProcessorIndex() == RD_ProcessorSwapper::ProcessorIndex::kGain);
    }

    SECTION("Setting active processor to kTDPSOLA is reflected by getter")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kTDPSOLA);
        REQUIRE(swapper.getActiveProcessorIndex() == RD_ProcessorSwapper::ProcessorIndex::kTDPSOLA);
    }

    SECTION("Setting active processor back to kGain is reflected by getter")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kTDPSOLA);
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGain);
        REQUIRE(swapper.getActiveProcessorIndex() == RD_ProcessorSwapper::ProcessorIndex::kGain);
    }

    SECTION("getActiveProcessor returns a non-null pointer for kGain")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGain);
        REQUIRE(swapper.getActiveProcessor() != nullptr);
    }

    SECTION("getActiveProcessor returns a non-null pointer for kTDPSOLA")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kTDPSOLA);
        REQUIRE(swapper.getActiveProcessor() != nullptr);
    }

    SECTION("getActiveProcessor returns the correct concrete type for kGain")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGain);
        REQUIRE(dynamic_cast<GainProcessor*> (swapper.getActiveProcessor()) != nullptr);
    }

    SECTION("getActiveProcessor returns the correct concrete type for kTDPSOLA")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kTDPSOLA);
        REQUIRE(dynamic_cast<TDPSOLA_Processor*> (swapper.getActiveProcessor()) != nullptr);
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

    SECTION("Fade state transitions to kFadingOut on first processBlock after setActiveProcessor")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kTDPSOLA);

        BufferFiller::fillWithAllOnes (buffer);
        swapper.processBlock (buffer, midi);

        REQUIRE(swapper.getFadeState() == Fade::FadeState::kFadingOut);
    }

    SECTION("Processor swap occurs after kFadeLength samples have been processed")
    {
        swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kTDPSOLA);

        // kFadeLength samples / blockSize samples per call = number of calls to complete fade
        const int numCalls = Fade::kFadeLength / blockSize;
        for (int i = 0; i < numCalls; ++i)
        {
            BufferFiller::fillWithAllOnes (buffer);
            swapper.processBlock (buffer, midi);
        }

        // After kFadeLength samples the fade value reaches 0, _applyProcessorSwap fires,
        // and triggerFadeIn is called — state should now be kFadingIn
        REQUIRE(swapper.getFadeState() == Fade::FadeState::kFadingIn);
    }
}
