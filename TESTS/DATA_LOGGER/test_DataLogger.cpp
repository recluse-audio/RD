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

TEST_CASE("DataLogger output file getter and setter", "[DataLogger]")
{
    DataLogger logger;

    SECTION("Set and get output file round-trips correctly")
    {
        juce::File outputDir ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT");
        logger.setOutputFile (outputDir);
        REQUIRE(logger.getOutputFile() == outputDir);
    }
}

TEST_CASE("DataLogger createDataLogFile writes expected content", "[DataLogger]")
{
    juce::File outputDir ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT");

    DataLogger logger;
    logger.setOutputFile (outputDir);

    auto logFile = logger.createDataLogFile();

    REQUIRE(logFile.existsAsFile());
    REQUIRE(logFile.loadFileAsString() == "DataLogger Default Output");
}

TEST_CASE("DataLogger logData returns false and creates no file when not logging", "[DataLogger]")
{
    juce::File outputDir ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT");

    DataLogger logger;
    logger.setOutputFile (outputDir);
    // mIsLogging defaults to false — no explicit set needed

    auto childrenBefore = outputDir.getNumberOfChildFiles (juce::File::findFilesAndDirectories);
    bool result = logger.logData();
    auto childrenAfter = outputDir.getNumberOfChildFiles (juce::File::findFilesAndDirectories);

    REQUIRE(result == false);
    REQUIRE(childrenAfter == childrenBefore);
}
