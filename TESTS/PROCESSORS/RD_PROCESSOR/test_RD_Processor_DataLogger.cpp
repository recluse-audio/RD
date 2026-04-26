#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/BASE/RD_Processor.h"
#include "../../../SOURCE/BUFFER_FILLER/BufferFiller.h"

//========================================================
//===================== DATA LOGGING =====================
//========================================================
// Protocol for DataLogger inheriters:
//   1. Build timestamped outputDir under
//      TESTS/PROCESSORS/<PROCESSOR>/OUTPUT/<TEST CASE NAME>/<timestamp>.
//      Treat outputDir as the parent directory for the logger.
//   2. Per SECTION, configure the logger:
//        processor.setParentDirectory(outputDir);
//        processor.setOutputDirectoryName("<section name>");
//        processor.createOutputDirectory();
//      so each section's logs are isolated under outputDir/<section name>/.
//   3. Log pre-process buffer, run processBlock, log post-process buffer,
//      then log processor state. REQUIRE each returned juce::File exists.

TEST_CASE("RD_Processor createDataLogFile writes processor name and APVTS XML", "[RD_Processor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_Processor processor;

    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/RD_PROCESSOR/OUTPUT/RD_Processor createDataLogFile writes processor name and APVTS XML")
                               .getChildFile (timestamp);

    processor.setParentDirectory (outputDir);
    processor.setOutputDirectoryName ("initial");
    processor.createOutputDirectory();

    auto logFile = processor.createProcessorDataLogFile();

    REQUIRE(logFile.existsAsFile());
    REQUIRE(logFile.getFileExtension() == ".xml");

    auto apvtsXml = juce::XmlDocument::parse (logFile);
    REQUIRE(apvtsXml != nullptr);
    REQUIRE(apvtsXml->getStringAttribute ("processorName") == processor.getName());

    auto* gainParam = apvtsXml->getChildByAttribute ("id", "gain");
    REQUIRE(gainParam != nullptr);
    REQUIRE(gainParam->getDoubleAttribute ("value") == Catch::Approx (1.0).margin (1e-5));

    processor.getAPVTS().getParameter ("gain")->setValueNotifyingHost (0.5f);

    juce::AudioBuffer<float> dummyBuffer (2, 512);
    juce::MidiBuffer dummyMidi;
    processor.processBlock (dummyBuffer, dummyMidi);

    processor.setOutputDirectoryName ("after-gain-change");
    processor.createOutputDirectory();

    auto logFile2 = processor.createProcessorDataLogFile();
    REQUIRE(logFile2.existsAsFile());

    auto apvtsXml2 = juce::XmlDocument::parse (logFile2);
    REQUIRE(apvtsXml2 != nullptr);

    auto* gainParam2 = apvtsXml2->getChildByAttribute ("id", "gain");
    REQUIRE(gainParam2 != nullptr);
    REQUIRE(gainParam2->getDoubleAttribute ("value") == Catch::Approx (0.5).margin (1e-5));
}

TEST_CASE("RD_Processor createProcessBlockDataLogFile writes audio buffer as CSV", "[RD_Processor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_Processor processor;

    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/RD_PROCESSOR/OUTPUT/RD_Processor createProcessBlockDataLogFile writes audio buffer as CSV")
                               .getChildFile (timestamp);
    processor.createOutputDirectory (outputDir);

    const int numChannels = 2;
    const int numSamples  = 256;
    juce::AudioBuffer<float> buffer (numChannels, numSamples);
    BufferFiller::fillIncremental (buffer);

    processor.setParentDirectory (outputDir);

    SECTION("Pre-processing flag produces preprocess_ file with index-equal samples")
    {
        processor.setOutputDirectoryName ("preprocess");
        processor.createOutputDirectory();

        auto logFile = processor.createProcessBlockDataLogFile (buffer, true);

        REQUIRE(logFile.existsAsFile());
        REQUIRE(logFile.getFileName().startsWith ("preprocess_"));
        REQUIRE(logFile.getFileExtension() == ".csv");

        auto lines = juce::StringArray::fromLines (logFile.loadFileAsString());
        REQUIRE(lines[0] == "ch0,ch1");
        REQUIRE(lines.size() >= numSamples + 1);

        for (int s = 0; s < numSamples; ++s)
        {
            auto row = juce::StringArray::fromTokens (lines[s + 1], ",", "");
            REQUIRE(row.size() == numChannels);
            for (int ch = 0; ch < numChannels; ++ch)
                REQUIRE(row[ch].getFloatValue() == Catch::Approx (static_cast<float> (s)).margin (1e-4));
        }
    }

    SECTION("Post-processing flag produces postprocess_ file")
    {
        processor.setOutputDirectoryName ("postprocess");
        processor.createOutputDirectory();

        auto logFile = processor.createProcessBlockDataLogFile (buffer, false);

        REQUIRE(logFile.existsAsFile());
        REQUIRE(logFile.getFileName().startsWith ("postprocess_"));
    }

    SECTION("Empty buffer writes header only")
    {
        processor.setOutputDirectoryName ("empty-buffer");
        processor.createOutputDirectory();

        juce::AudioBuffer<float> empty (1, 0);
        auto logFile = processor.createProcessBlockDataLogFile (empty, true);

        REQUIRE(logFile.existsAsFile());
        auto lines = juce::StringArray::fromLines (logFile.loadFileAsString());
        REQUIRE(lines[0] == "ch0");
    }
}

TEST_CASE("RD_Processor parent logs propagate to child RD_Processor", "[RD_Processor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_Processor parent;
    RD_Processor child;

    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/RD_PROCESSOR/OUTPUT/RD_Processor parent logs propagate to child RD_Processor")
                               .getChildFile (timestamp);

    parent.setParentDirectory (outputDir);
    parent.setOutputDirectoryName ("parent");
    parent.createOutputDirectory();

    child.setParentDirectory (outputDir);
    child.setOutputDirectoryName ("child");
    child.createOutputDirectory();

    parent.setIsLogging (true);
    child.setIsLogging (true);

    parent.addChild (&child);

    SECTION("Both parent and child write log files via parent.logData()")
    {
        REQUIRE(parent.getNumChildren() == 1);
        REQUIRE(parent.logData() == true);

        auto parentLog = parent.getOutputDirectory().getChildFile ("output.txt");
        auto childLog  = child.getOutputDirectory().getChildFile ("output.txt");

        REQUIRE(parentLog.existsAsFile());
        REQUIRE(childLog.existsAsFile());
    }

    SECTION("Removing child stops propagation")
    {
        parent.removeChild (&child);

        // Wipe child dir to detect any unexpected write.
        child.getOutputDirectory().deleteRecursively();
        child.createOutputDirectory();

        REQUIRE(parent.getNumChildren() == 0);
        REQUIRE(parent.logData() == true);

        auto parentLog = parent.getOutputDirectory().getChildFile ("output.txt");
        auto childLog  = child.getOutputDirectory().getChildFile ("output.txt");

        REQUIRE(parentLog.existsAsFile());
        REQUIRE_FALSE(childLog.existsAsFile());
    }
}
