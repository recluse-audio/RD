#include <catch2/catch_test_macros.hpp>
#include "../SOURCE/PROCESSORS/RD_ProcessorSwapper.h"
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
