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
    logger.setDataLogRootDirectory (caseDir);
    logger.setDataLogOutputName ("default");
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

TEST_CASE("DataLogger root + name compose output directory", "[DataLogger]")
{
    auto caseDir = makeCaseDir ("DataLogger root + name compose output directory");

    DataLogger logger;

    SECTION("setDataLogRootDirectory + setDataLogOutputName build expected getDataLogOutputDirectory()")
    {
        logger.setDataLogRootDirectory (caseDir);
        logger.setDataLogOutputName ("my-section");

        REQUIRE(logger.getDataLogRootDirectory() == caseDir);
        REQUIRE(logger.getDataLogOutputName() == "my-section");
        REQUIRE(logger.getDataLogParentDirectory() == caseDir);
        REQUIRE(logger.getDataLogOutputDirectory() == caseDir.getChildFile ("my-section"));
    }

    SECTION("Default output name is a non-empty timestamp")
    {
        DataLogger fresh;
        REQUIRE(fresh.getDataLogOutputName().isNotEmpty());
    }
}

TEST_CASE("DataLogger logData materializes output directory and writes default file", "[DataLogger]")
{
    auto caseDir = makeCaseDir ("DataLogger logData materializes output directory and writes default file");

    DataLogger logger;
    logger.setDataLogRootDirectory (caseDir);
    logger.setDataLogOutputName ("default");
    logger.setIsLogging (true);

    REQUIRE(logger.logData() == true);

    auto outputDir = logger.getDataLogOutputDirectory();
    REQUIRE(outputDir.isDirectory());

    auto logFile = outputDir.getChildFile ("output.txt");
    REQUIRE(logFile.existsAsFile());
    REQUIRE(logFile.loadFileAsString() == "DataLogger Default Output");
}

namespace
{
    class CountingLogger : public DataLogger
    {
    public:
        int callCount = 0;

        juce::File _createDataLogEventFile() override
        {
            ++callCount;
            return DataLogger::_createDataLogEventFile();
        }
    };

    void configureLogger (DataLogger& logger, const juce::File& root, const juce::String& name)
    {
        logger.setDataLogRootDirectory (root);
        logger.setDataLogOutputName (name);
    }
}

TEST_CASE("DataLogger addChild stores parent back-pointer and routes child path through parent", "[DataLogger]")
{
    auto caseDir = makeCaseDir ("DataLogger addChild stores parent back-pointer and routes child path through parent");

    DataLogger parent;
    CountingLogger child;

    configureLogger (parent, caseDir, "parent");

    auto unrelatedDir = caseDir.getChildFile ("somewhere-else");
    child.setDataLogRootDirectory (unrelatedDir);
    child.setDataLogOutputName ("child");

    SECTION("Without parent logger, child path uses its own root")
    {
        REQUIRE(child.getParentLogger() == nullptr);
        REQUIRE(child.getDataLogParentDirectory() == unrelatedDir);
        REQUIRE(child.getDataLogOutputDirectory() == unrelatedDir.getChildFile ("child"));
    }

    SECTION("addChild stores back-pointer and reroutes child parent dir to parent's output dir")
    {
        parent.addChild (&child);

        REQUIRE(child.getParentLogger() == &parent);
        REQUIRE(child.getDataLogParentDirectory() == parent.getDataLogOutputDirectory());
        REQUIRE(child.getDataLogOutputDirectory() == parent.getDataLogOutputDirectory().getChildFile ("child"));
    }

    SECTION("parent.logData() cascades and creates child output dir under parent")
    {
        parent.addChild (&child);
        parent.setIsLogging (true);
        child.setIsLogging (true);

        parent.logData();

        REQUIRE(child.callCount == 1);
        REQUIRE(child.getDataLogOutputDirectory().getChildFile ("output.txt").existsAsFile());
        REQUIRE(child.getDataLogOutputDirectory() == parent.getDataLogOutputDirectory().getChildFile ("child"));
    }

    SECTION("Renaming parent's output name between logs follows on the next log")
    {
        parent.addChild (&child);
        parent.setIsLogging (true);
        child.setIsLogging (true);

        parent.logData();
        auto firstChildPath = child.getDataLogOutputDirectory();

        parent.setDataLogOutputName ("parent-renamed");

        parent.logData();
        auto secondChildPath = child.getDataLogOutputDirectory();

        REQUIRE(firstChildPath != secondChildPath);
        REQUIRE(secondChildPath == parent.getDataLogOutputDirectory().getChildFile ("child"));
        REQUIRE(secondChildPath.getChildFile ("output.txt").existsAsFile());
    }

    SECTION("removeChild clears the back-pointer; child reverts to its own root")
    {
        parent.addChild (&child);
        parent.removeChild (&child);

        REQUIRE(child.getParentLogger() == nullptr);

        child.setDataLogRootDirectory (caseDir.getChildFile ("orphan"));
        child.setIsLogging (true);

        REQUIRE(child.logData() == true);
        REQUIRE(child.getDataLogParentDirectory() == caseDir.getChildFile ("orphan"));
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

    childA.setIsLogging (true);
    childB.setIsLogging (true);
    parent.setIsLogging (false);

    parent.addChild (&childA);
    parent.addChild (&childB);

    SECTION("Parent off short-circuits — no child _createDataLogEventFile call")
    {
        REQUIRE(parent.logData() == false);
        REQUIRE(parent.callCount == 0);
        REQUIRE(childA.callCount == 0);
        REQUIRE(childB.callCount == 0);
    }

    SECTION("No log files written under any logger directory")
    {
        parent.logData();

        REQUIRE_FALSE(parent.getDataLogOutputDirectory().getChildFile ("output.txt").existsAsFile());
        REQUIRE_FALSE(childA.getDataLogOutputDirectory().getChildFile ("output.txt").existsAsFile());
        REQUIRE_FALSE(childB.getDataLogOutputDirectory().getChildFile ("output.txt").existsAsFile());
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

    bool result = logger.logData();

    REQUIRE(result == false);
    REQUIRE_FALSE(logger.getDataLogOutputDirectory().exists());
}
