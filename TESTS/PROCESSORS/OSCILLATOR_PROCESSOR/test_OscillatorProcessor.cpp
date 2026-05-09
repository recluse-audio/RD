#include <catch2/catch_test_macros.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/OSCILLATOR/OscillatorProcessor.h"

TEST_CASE("OscillatorProcessor reports its name", "[OscillatorProcessor]")
{
    TestUtils::SetupAndTeardown setup;
    OscillatorProcessor processor;

    REQUIRE(processor.getName() == "Oscillator Processor");
}
