#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/BASE/RD_Processor.h"

TEST_CASE("RD_Processor caches sample rate and block size from prepareToPlay", "[RD_Processor]")
{
    TestUtils::SetupAndTeardown setup;
    RD_Processor processor;

    SECTION("Defaults before prepareToPlay")
    {
        REQUIRE(processor.getLastSampleRateFromPrepareToPlay() == 44100.0);
        REQUIRE(processor.getLastBlockSizeFromPrepareToPlay()  == 512);
    }

    SECTION("Values are cached after prepareToPlay")
    {
        processor.prepareToPlay(48000.0, 256);
        REQUIRE(processor.getLastSampleRateFromPrepareToPlay() == 48000.0);
        REQUIRE(processor.getLastBlockSizeFromPrepareToPlay()  == 256);
    }

    SECTION("Subsequent prepareToPlay calls overwrite cached values")
    {
        processor.prepareToPlay(48000.0, 256);
        processor.prepareToPlay(96000.0, 1024);
        REQUIRE(processor.getLastSampleRateFromPrepareToPlay() == 96000.0);
        REQUIRE(processor.getLastBlockSizeFromPrepareToPlay()  == 1024);
    }
}

TEST_CASE("RD_Processor tracks process sample count and resets on prepareToPlay", "[RD_Processor]")
{
    TestUtils::SetupAndTeardown setup;
    RD_Processor processor;

    SECTION("Default is zero before any prepareToPlay")
    {
        REQUIRE(processor.getProcessSampleCount() == 0);
    }

    SECTION("prepareToPlay resets count to zero")
    {
        processor.prepareToPlay (44100.0, 512);
        REQUIRE(processor.getProcessSampleCount() == 0);
    }

    SECTION("prepareToPlay clears a non-zero count")
    {
        // Simulate work done by subclasses bumping the count via the protected
        // member — exercised here through a derived test fixture.
        struct CountingProcessor : public RD_Processor
        {
            void bump (int n) { mProcessSampleCount += n; }
        };

        CountingProcessor counter;
        counter.bump (1024);
        REQUIRE(counter.getProcessSampleCount() == 1024);

        counter.prepareToPlay (48000.0, 256);
        REQUIRE(counter.getProcessSampleCount() == 0);
    }
}
