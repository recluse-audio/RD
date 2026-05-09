#include <catch2/catch_test_macros.hpp>
#include "../../SOURCE/DATA_LOGGER/DataLogger.h"

namespace
{
    // Path layout:
    //   OUTPUT/<TEST_NAME>[/<SECTION_NAME>]/TEST_CASE_ROOT_DIR/DATA_LOG_OUTPUT_DIR
    juce::File makeRootDir (const juce::String& testName, const juce::String& sectionName = {})
    {
        auto base = juce::File (__FILE__).getParentDirectory()
                                         .getChildFile ("OUTPUT")
                                         .getChildFile (testName);
        if (sectionName.isNotEmpty())
            base = base.getChildFile (sectionName);
        return base.getChildFile ("TEST_CASE_ROOT_DIR");
    }

    juce::String makeOutputName()
    {
        return "DATA_LOG_OUTPUT_DIR";
    }
}

TEST_CASE("DataLogger constructs without error", "[DataLogger]")
{
    auto rootDir = makeRootDir ("DataLogger constructs without error");

    DataLogger logger;
    logger.setDataLogRootDirectory (rootDir);
    logger.setDataLogOutputName (makeOutputName());
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

TEST_CASE("DataLogger setIsLogging cascades to registered children", "[DataLogger]")
{
    DataLogger parent;
    DataLogger childA;
    DataLogger childB;
    DataLogger grandchild;

    parent.addChild (&childA);
    parent.addChild (&childB);
    childA.addChild (&grandchild);

    REQUIRE_FALSE (parent    .getIsLogging());
    REQUIRE_FALSE (childA    .getIsLogging());
    REQUIRE_FALSE (childB    .getIsLogging());
    REQUIRE_FALSE (grandchild.getIsLogging());

    SECTION("setIsLogging(true) on parent flips every descendant")
    {
        parent.setIsLogging (true);
        REQUIRE (parent    .getIsLogging());
        REQUIRE (childA    .getIsLogging());
        REQUIRE (childB    .getIsLogging());
        REQUIRE (grandchild.getIsLogging());
    }

    SECTION("setIsLogging(false) on parent flips every descendant back")
    {
        parent.setIsLogging (true);
        parent.setIsLogging (false);
        REQUIRE_FALSE (parent    .getIsLogging());
        REQUIRE_FALSE (childA    .getIsLogging());
        REQUIRE_FALSE (childB    .getIsLogging());
        REQUIRE_FALSE (grandchild.getIsLogging());
    }

    SECTION("setIsLogging on a child does not flip its parent or sibling")
    {
        childA.setIsLogging (true);
        REQUIRE       (childA    .getIsLogging());
        REQUIRE       (grandchild.getIsLogging());
        REQUIRE_FALSE (parent    .getIsLogging());
        REQUIRE_FALSE (childB    .getIsLogging());
    }

    SECTION("removed child no longer cascades")
    {
        parent.removeChild (&childA);
        parent.setIsLogging (true);
        REQUIRE       (parent    .getIsLogging());
        REQUIRE       (childB    .getIsLogging());
        REQUIRE_FALSE (childA    .getIsLogging());
        REQUIRE_FALSE (grandchild.getIsLogging());
    }
}

TEST_CASE("DataLogger root + name compose output directory", "[DataLogger]")
{
    DataLogger logger;

    SECTION("setDataLogRootDirectory + setDataLogOutputName build expected getDataLogOutputDirectory()")
    {
        auto rootDir    = makeRootDir ("DataLogger root + name compose output directory",
                                       "setDataLogRootDirectory_setDataLogOutputName_build_expected");
        auto outputName = makeOutputName();
        logger.setDataLogRootDirectory (rootDir);
        logger.setDataLogOutputName (outputName);

        REQUIRE(logger.getDataLogRootDirectory() == rootDir);
        REQUIRE(logger.getDataLogOutputName() == outputName);
        REQUIRE(logger.getDataLogParentDirectory() == rootDir);
        REQUIRE(logger.getDataLogOutputDirectory() == rootDir.getChildFile (outputName));
    }

    SECTION("Default output name is a non-empty timestamp")
    {
        DataLogger fresh;
        REQUIRE(fresh.getDataLogOutputName().isNotEmpty());
    }
}

TEST_CASE("DataLogger logData materializes output directory and writes default file", "[DataLogger]")
{
    auto rootDir = makeRootDir ("DataLogger logData materializes output directory and writes default file");

    DataLogger logger;
    logger.setDataLogRootDirectory (rootDir);
    logger.setDataLogOutputName (makeOutputName());
    logger.setIsLogging (true);

    REQUIRE(logger.logData() == true);

    auto outputDir = logger.getDataLogOutputDirectory();
    REQUIRE(outputDir.isDirectory());

    auto logFile = outputDir.getChildFile ("data_log_event.txt");
    REQUIRE(logFile.existsAsFile());
    REQUIRE(logFile.loadFileAsString().startsWith ("Log Time: "));
}

namespace
{
    class CountingLogger : public DataLogger
    {
    public:
        int callCount = 0;

        bool doLogData() override
        {
            ++callCount;
            return DataLogger::doLogData();
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
    auto rootDir = makeRootDir ("DataLogger addChild stores parent back-pointer and routes child path through parent");

    DataLogger parent;
    CountingLogger child;

    configureLogger (parent, rootDir, makeOutputName());

    auto unrelatedDir = rootDir.getChildFile ("somewhere-else");
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
        REQUIRE(child.getDataLogOutputDirectory().getChildFile ("data_log_event.txt").existsAsFile());
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
        REQUIRE(secondChildPath.getChildFile ("data_log_event.txt").existsAsFile());
    }

    SECTION("removeChild clears the back-pointer; child reverts to its own root")
    {
        parent.addChild (&child);
        parent.removeChild (&child);

        REQUIRE(child.getParentLogger() == nullptr);

        child.setDataLogRootDirectory (rootDir.getChildFile ("orphan"));
        child.setIsLogging (true);

        REQUIRE(child.logData() == true);
        REQUIRE(child.getDataLogParentDirectory() == rootDir.getChildFile ("orphan"));
    }
}

TEST_CASE("DataLogger addChild propagates logData to children", "[DataLogger]")
{
    auto rootDir = makeRootDir ("DataLogger addChild propagates logData to children");

    CountingLogger parent;
    CountingLogger childA;
    CountingLogger childB;

    configureLogger (parent, rootDir, makeOutputName());
    configureLogger (childA, rootDir, "childA");
    configureLogger (childB, rootDir, "childB");

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
    auto rootDir = makeRootDir ("DataLogger parent logging off skips children");
    rootDir.deleteRecursively(); // clear leftovers from prior runs (other sections write files)

    CountingLogger parent;
    CountingLogger childA;
    CountingLogger childB;

    configureLogger (parent, rootDir, makeOutputName());
    configureLogger (childA, rootDir, "childA");
    configureLogger (childB, rootDir, "childB");

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

        REQUIRE_FALSE(parent.getDataLogOutputDirectory().getChildFile ("data_log_event.txt").existsAsFile());
        REQUIRE_FALSE(childA.getDataLogOutputDirectory().getChildFile ("data_log_event.txt").existsAsFile());
        REQUIRE_FALSE(childB.getDataLogOutputDirectory().getChildFile ("data_log_event.txt").existsAsFile());
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
    auto rootDir = makeRootDir ("DataLogger logData returns false and creates no file when not logging");

    DataLogger logger;
    configureLogger (logger, rootDir, makeOutputName());
    // mIsLogging defaults to false — no explicit set needed

    bool result = logger.logData();

    REQUIRE(result == false);
    REQUIRE_FALSE(logger.getDataLogOutputDirectory().exists());
}
