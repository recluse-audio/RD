#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/GRAIN/GrainShifterProcessor.h"

//========================================================
//===================== DATA LOGGING =====================
//========================================================
// See GAIN_PROCESSOR/test_GainProcessor_DataLogger.cpp for protocol.

TEST_CASE("GrainShifterProcessor prepareToPlay logs sampleRate and maxBlockSize", "[GrainShifterProcessor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("GrainShifterProcessor prepareToPlay logs sampleRate and maxBlockSize")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");

    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    GrainShifterProcessor processor;
    processor.setDataLogRootDirectory (rootDir);
    processor.setDataLogOutputName    (outputName);
    processor.startLogging();

    const double sampleRate   = 48000.0;
    const int    maxBlockSize = 1024;
    processor.prepareToPlay (sampleRate, maxBlockSize);

    auto outputDir = processor.getDataLogOutputDirectory();
    auto prepFile  = outputDir.getChildFile ("prepare_to_play").getChildFile ("prepare_to_play.csv");
    REQUIRE (prepFile.existsAsFile());

    auto lines = juce::StringArray::fromLines (prepFile.loadFileAsString().trimEnd());
    REQUIRE (lines.size() == 2);
    REQUIRE (lines[0] == "sampleRate,maxBlockSize");

    auto values = juce::StringArray::fromTokens (lines[1], ",", "");
    REQUIRE (values.size() == 2);
    REQUIRE (values[0].getDoubleValue() == Catch::Approx (sampleRate));
    REQUIRE (values[1].getIntValue()    == maxBlockSize);

    processor.stopLogging();
}

TEST_CASE("GrainShifterProcessor::createProcessorDataLogFile", "[GrainShifterProcessor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File testDir = juce::File (__FILE__).getParentDirectory()
                                               .getChildFile ("OUTPUT")
                                               .getChildFile ("GrainShifterProcessor_createProcessorDataLogFile");

    auto makeSectionRoot = [&] (const juce::String& sectionName)
    {
        return testDir.getChildFile (sectionName).getChildFile ("TEST_CASE_ROOT_DIR");
    };
    auto makeOutputName = [] ()
    {
        return juce::String ("DATA_LOG_OUTPUT_DIR");
    };

    auto readFloatParamFromXml = [] (const juce::File& file, const juce::String& paramID) -> float
    {
        auto xml = juce::XmlDocument::parse (file);
        REQUIRE (xml != nullptr);
        for (auto* child : xml->getChildIterator())
        {
            if (child->hasTagName ("PARAM") && child->getStringAttribute ("id") == paramID)
                return static_cast<float> (child->getDoubleAttribute ("value"));
        }
        FAIL ("PARAM '" + paramID + "' not found in processor_state.xml");
        return 0.0f;
    };

    SECTION("Default shift_ratio = 1.0 written to processor_state.xml")
    {
        GrainShifterProcessor processor;
        processor.setDataLogRootDirectory (makeSectionRoot ("default"));
        processor.setDataLogOutputName (makeOutputName());
        processor.startLogging();

        auto stateFile = processor.createProcessorDataLogFile();
        REQUIRE (stateFile.existsAsFile());
        REQUIRE (stateFile == processor.getDataLogOutputDirectory().getChildFile ("processor_state.xml"));

        auto xml = juce::XmlDocument::parse (stateFile);
        REQUIRE (xml != nullptr);
        REQUIRE (xml->getStringAttribute ("processorName") == processor.getName());

        REQUIRE (readFloatParamFromXml (stateFile, "shift_ratio") == Catch::Approx (1.0f));

        processor.stopLogging();
    }

    SECTION("Modified shift_ratio = 1.5 written to processor_state.xml")
    {
        GrainShifterProcessor processor;
        processor.setDataLogRootDirectory (makeSectionRoot ("modified"));
        processor.setDataLogOutputName (makeOutputName());
        processor.startLogging();

        auto* shiftRatioParam = processor.getAPVTS().getParameter ("shift_ratio");
        REQUIRE (shiftRatioParam != nullptr);
        // Range is 0.5..2.0; normalized 1.5 maps to denormalized 1.5.
        const float denormalizedTarget = 1.5f;
        const float normalized         = (denormalizedTarget - 0.5f) / (2.0f - 0.5f);
        shiftRatioParam->setValueNotifyingHost (normalized);

        auto stateFile = processor.createProcessorDataLogFile();
        REQUIRE (stateFile.existsAsFile());

        REQUIRE (readFloatParamFromXml (stateFile, "shift_ratio") == Catch::Approx (denormalizedTarget).margin (1e-3));

        processor.stopLogging();
    }
}
