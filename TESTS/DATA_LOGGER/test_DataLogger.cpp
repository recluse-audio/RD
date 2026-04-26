#include <catch2/catch_test_macros.hpp>
#include "../../SOURCE/DATA_LOGGER/DataLogger.h"

TEST_CASE("DataLogger constructs without error", "[DataLogger]")
{
    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT/DataLogger constructs without error")
                               .getChildFile (timestamp);

    DataLogger logger;
    logger.createOutputDirectory (outputDir);
    logger.setOutputFile (outputDir);
    SUCCEED();
}

TEST_CASE("DataLogger mIsLogging getter and setter", "[DataLogger]")
{
    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT/DataLogger mIsLogging getter and setter")
                               .getChildFile (timestamp);

    DataLogger logger;
    logger.createOutputDirectory (outputDir);
    logger.setOutputFile (outputDir);

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
    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT/DataLogger output file getter and setter")
                               .getChildFile (timestamp);

    DataLogger logger;
    logger.createOutputDirectory (outputDir);

    SECTION("Set and get output file round-trips correctly")
    {
        logger.setOutputFile (outputDir);
        REQUIRE(logger.getOutputFile() == outputDir);
    }
}

TEST_CASE("DataLogger createDataLogFile writes expected content", "[DataLogger]")
{
    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT/DataLogger createDataLogFile writes expected content")
                               .getChildFile (timestamp);

    DataLogger logger;
    logger.createOutputDirectory (outputDir);
    logger.setOutputFile (outputDir);

    auto logFile = logger.createDataLogFile();

    REQUIRE(logFile.existsAsFile());
    REQUIRE(logFile.loadFileAsString() == "DataLogger Default Output");
}

namespace
{
    class CountingLogger : public DataLogger
    {
    public:
        int callCount = 0;

        juce::File createDataLogFile() override
        {
            ++callCount;
            return DataLogger::createDataLogFile();
        }
    };
}

TEST_CASE("DataLogger addChild propagates logData to children", "[DataLogger]")
{
    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT/DataLogger addChild propagates logData to children")
                               .getChildFile (timestamp);

    CountingLogger parent;
    CountingLogger childA;
    CountingLogger childB;

    parent.createOutputDirectory (outputDir.getChildFile ("parent"));
    childA.createOutputDirectory (outputDir.getChildFile ("childA"));
    childB.createOutputDirectory (outputDir.getChildFile ("childB"));

    parent.setOutputFile (outputDir.getChildFile ("parent"));
    childA.setOutputFile (outputDir.getChildFile ("childA"));
    childB.setOutputFile (outputDir.getChildFile ("childB"));

    parent.setIsLogging (true);
    childA.setIsLogging (true);
    childB.setIsLogging (true);

    parent.addChild (&childA);
    parent.addChild (&childB);

    SECTION("Both children logged")
    {
        REQUIRE(parent.getNumChildren() == 2);
        REQUIRE(parent.logData() == true);
        REQUIRE(parent.callCount == 1);
        REQUIRE(childA.callCount == 1);
        REQUIRE(childB.callCount == 1);
    }

    SECTION("Duplicate add is ignored")
    {
        parent.addChild (&childA);
        REQUIRE(parent.getNumChildren() == 2);
    }

    SECTION("Self-add is ignored")
    {
        parent.addChild (&parent);
        REQUIRE(parent.getNumChildren() == 2);
    }

    SECTION("removeChild stops propagation")
    {
        parent.removeChild (&childA);
        REQUIRE(parent.getNumChildren() == 1);
        parent.logData();
        REQUIRE(childA.callCount == 0);
        REQUIRE(childB.callCount == 1);
    }

    SECTION("Child not logging skipped without failing parent")
    {
        childA.setIsLogging (false);
        REQUIRE(parent.logData() == false); // childA returns false
        REQUIRE(childA.callCount == 0);
        REQUIRE(childB.callCount == 1);
    }
}

TEST_CASE("DataLogger parent logging off skips children", "[DataLogger]")
{
    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT/DataLogger parent logging off skips children")
                               .getChildFile (timestamp);

    CountingLogger parent;
    CountingLogger childA;
    CountingLogger childB;

    parent.createOutputDirectory (outputDir.getChildFile ("parent"));
    childA.createOutputDirectory (outputDir.getChildFile ("childA"));
    childB.createOutputDirectory (outputDir.getChildFile ("childB"));

    parent.setOutputFile (outputDir.getChildFile ("parent"));
    childA.setOutputFile (outputDir.getChildFile ("childA"));
    childB.setOutputFile (outputDir.getChildFile ("childB"));

    // Children individually want to log, but parent is off.
    childA.setIsLogging (true);
    childB.setIsLogging (true);
    parent.setIsLogging (false);

    parent.addChild (&childA);
    parent.addChild (&childB);

    SECTION("Parent off short-circuits — no child createDataLogFile call")
    {
        REQUIRE(parent.logData() == false);
        REQUIRE(parent.callCount == 0);
        REQUIRE(childA.callCount == 0);
        REQUIRE(childB.callCount == 0);
    }

    SECTION("No log files written under any logger dir")
    {
        parent.logData();

        auto parentLog = outputDir.getChildFile ("parent").getChildFile ("output.txt");
        auto childALog = outputDir.getChildFile ("childA").getChildFile ("output.txt");
        auto childBLog = outputDir.getChildFile ("childB").getChildFile ("output.txt");

        REQUIRE_FALSE(parentLog.existsAsFile());
        REQUIRE_FALSE(childALog.existsAsFile());
        REQUIRE_FALSE(childBLog.existsAsFile());
    }

    SECTION("Re-enabling parent restores cascade")
    {
        parent.logData(); // off — no calls
        parent.setIsLogging (true);

        REQUIRE(parent.logData() == true);
        REQUIRE(parent.callCount == 1);
        REQUIRE(childA.callCount == 1);
        REQUIRE(childB.callCount == 1);
    }
}

TEST_CASE("DataLogger logData returns false and creates no file when not logging", "[DataLogger]")
{
    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT/DataLogger logData returns false and creates no file when not logging")
                               .getChildFile (timestamp);

    DataLogger logger;
    logger.createOutputDirectory (outputDir);
    logger.setOutputFile (outputDir);
    // mIsLogging defaults to false — no explicit set needed

    auto childrenBefore = outputDir.getNumberOfChildFiles (juce::File::findFilesAndDirectories);
    bool result = logger.logData();
    auto childrenAfter = outputDir.getNumberOfChildFiles (juce::File::findFilesAndDirectories);

    REQUIRE(result == false);
    REQUIRE(childrenAfter == childrenBefore);
}
