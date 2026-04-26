#include <catch2/catch_test_macros.hpp>
#include "../../SOURCE/DATA_LOGGER/DataLogger.h"

namespace
{
    juce::File makeCaseDir (const juce::String& caseName)
    {
        auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
        return juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT/" + caseName)
                   .getChildFile (timestamp);
    }
}

TEST_CASE("DataLogger constructs without error", "[DataLogger]")
{
    auto caseDir = makeCaseDir ("DataLogger constructs without error");

    DataLogger logger;
    logger.setParentDirectory (caseDir);
    logger.setOutputDirectoryName ("default");
    logger.createOutputDirectory();
    SUCCEED();
}

TEST_CASE("DataLogger mIsLogging getter and setter", "[DataLogger]")
{
    auto caseDir = makeCaseDir ("DataLogger mIsLogging getter and setter");

    DataLogger logger;
    logger.setParentDirectory (caseDir);
    logger.setOutputDirectoryName ("default");
    logger.createOutputDirectory();

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

TEST_CASE("DataLogger parent directory and output directory name round-trip", "[DataLogger]")
{
    auto caseDir = makeCaseDir ("DataLogger parent directory and output directory name round-trip");

    DataLogger logger;

    SECTION("setParentDirectory + setOutputDirectoryName build expected getOutputDirectory()")
    {
        logger.setParentDirectory (caseDir);
        logger.setOutputDirectoryName ("my-section");

        REQUIRE(logger.getParentDirectory() == caseDir);
        REQUIRE(logger.getOutputDirectoryName() == "my-section");
        REQUIRE(logger.getOutputDirectory() == caseDir.getChildFile ("my-section"));
    }

    SECTION("createOutputDirectory() materializes mParentDirectory/mOutputDirectoryName on disk")
    {
        logger.setParentDirectory (caseDir);
        logger.setOutputDirectoryName ("created");

        REQUIRE(logger.createOutputDirectory());
        REQUIRE(logger.getOutputDirectory().isDirectory());
    }
}

TEST_CASE("DataLogger createDataLogFile writes expected content", "[DataLogger]")
{
    auto caseDir = makeCaseDir ("DataLogger createDataLogFile writes expected content");

    DataLogger logger;
    logger.setParentDirectory (caseDir);
    logger.setOutputDirectoryName ("default");
    logger.createOutputDirectory();

    auto logFile = logger.createDataLogFile();

    REQUIRE(logFile.existsAsFile());
    REQUIRE(logFile.getParentDirectory() == logger.getOutputDirectory());
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

    void configureLogger (DataLogger& logger, const juce::File& parent, const juce::String& name)
    {
        logger.setParentDirectory (parent);
        logger.setOutputDirectoryName (name);
        logger.createOutputDirectory();
    }
}

TEST_CASE("DataLogger addChild stores parent back-pointer and syncs child path at logData time", "[DataLogger]")
{
    auto caseDir = makeCaseDir ("DataLogger addChild stores parent back-pointer and syncs child path at logData time");

    DataLogger parent;
    CountingLogger child;

    configureLogger (parent, caseDir, "parent");

    // Child starts with an unrelated parent directory.
    auto unrelatedDir = caseDir.getChildFile ("somewhere-else");
    child.setParentDirectory (unrelatedDir);
    child.setOutputDirectoryName ("child");

    SECTION("addChild stores back-pointer but does not mutate parent directory yet")
    {
        REQUIRE(child.getParentLogger() == nullptr);

        parent.addChild (&child);

        REQUIRE(child.getParentLogger() == &parent);
        // Child path untouched until logData runs.
        REQUIRE(child.getParentDirectory() == unrelatedDir);
    }

    SECTION("child.logData() syncs parent directory from parent logger")
    {
        parent.addChild (&child);
        parent.setIsLogging (true);
        child.setIsLogging (true);

        parent.logData();

        REQUIRE(child.getParentDirectory() == parent.getOutputDirectory());
        REQUIRE(child.getOutputDirectory() == parent.getOutputDirectory().getChildFile ("child"));
        REQUIRE(child.callCount == 1);
        REQUIRE(child.getOutputDirectory().getChildFile ("output.txt").existsAsFile());
    }

    SECTION("Renaming parent's output directory between logs follows on the next log")
    {
        parent.addChild (&child);
        parent.setIsLogging (true);
        child.setIsLogging (true);

        parent.logData();
        auto firstChildPath = child.getOutputDirectory();

        parent.setOutputDirectoryName ("parent-renamed");
        parent.createOutputDirectory();

        parent.logData();
        auto secondChildPath = child.getOutputDirectory();

        REQUIRE(firstChildPath != secondChildPath);
        REQUIRE(secondChildPath == parent.getOutputDirectory().getChildFile ("child"));
        REQUIRE(secondChildPath.getChildFile ("output.txt").existsAsFile());
    }

    SECTION("removeChild clears the back-pointer; subsequent direct child.logData() uses child's own parent dir")
    {
        parent.addChild (&child);
        parent.removeChild (&child);

        REQUIRE(child.getParentLogger() == nullptr);

        // Restore an isolated child path so it can log on its own.
        child.setParentDirectory (caseDir.getChildFile ("orphan"));
        child.setIsLogging (true);

        REQUIRE(child.logData() == true);
        REQUIRE(child.getParentDirectory() == caseDir.getChildFile ("orphan"));
    }
}

TEST_CASE("DataLogger addChild propagates logData to children", "[DataLogger]")
{
    auto caseDir = makeCaseDir ("DataLogger addChild propagates logData to children");

    CountingLogger parent;
    CountingLogger childA;
    CountingLogger childB;

    configureLogger (parent, caseDir, "parent");
    configureLogger (childA, caseDir, "childA");
    configureLogger (childB, caseDir, "childB");

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
    auto caseDir = makeCaseDir ("DataLogger parent logging off skips children");

    CountingLogger parent;
    CountingLogger childA;
    CountingLogger childB;

    configureLogger (parent, caseDir, "parent");
    configureLogger (childA, caseDir, "childA");
    configureLogger (childB, caseDir, "childB");

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

    SECTION("No log files written under any logger directory")
    {
        parent.logData();

        REQUIRE_FALSE(parent.getOutputDirectory().getChildFile ("output.txt").existsAsFile());
        REQUIRE_FALSE(childA.getOutputDirectory().getChildFile ("output.txt").existsAsFile());
        REQUIRE_FALSE(childB.getOutputDirectory().getChildFile ("output.txt").existsAsFile());
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
    auto caseDir = makeCaseDir ("DataLogger logData returns false and creates no file when not logging");

    DataLogger logger;
    configureLogger (logger, caseDir, "default");
    // mIsLogging defaults to false — no explicit set needed

    auto outputDir = logger.getOutputDirectory();
    auto childrenBefore = outputDir.getNumberOfChildFiles (juce::File::findFilesAndDirectories);
    bool result = logger.logData();
    auto childrenAfter = outputDir.getNumberOfChildFiles (juce::File::findFilesAndDirectories);

    REQUIRE(result == false);
    REQUIRE(childrenAfter == childrenBefore);
}
