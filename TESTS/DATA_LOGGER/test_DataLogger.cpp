#include <catch2/catch_test_macros.hpp>
#include "../../SOURCE/DATA_LOGGER/DataLogger.h"

TEST_CASE("DataLogger constructs without error", "[DataLogger]")
{
    DataLogger logger;
    SUCCEED();
}

TEST_CASE("DataLogger mIsLogging getter and setter", "[DataLogger]")
{
    DataLogger logger;

    SECTION("Default is false")
    {
        REQUIRE(logger.getIsLogging() == false);
    }

    SECTION("Set true")
    {
        logger.setIsLogging(true);
        REQUIRE(logger.getIsLogging() == true);
    }

    SECTION("Set false after true")
    {
        logger.setIsLogging(true);
        logger.setIsLogging(false);
        REQUIRE(logger.getIsLogging() == false);
    }
}
