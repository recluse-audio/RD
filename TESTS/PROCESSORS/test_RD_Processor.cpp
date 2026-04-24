#include <catch2/catch_test_macros.hpp>
#include "../TEST_UTILS/TestUtils.h"
#include "../../SOURCE/PROCESSORS/BASE/RD_Processor.h"

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
