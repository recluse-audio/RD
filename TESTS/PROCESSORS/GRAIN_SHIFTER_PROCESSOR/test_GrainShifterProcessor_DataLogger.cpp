#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/GRAIN/GrainShifterProcessor.h"

//========================================================
//===================== DATA LOGGING =====================
//========================================================
// See GAIN_PROCESSOR/test_GainProcessor_DataLogger.cpp for protocol.

TEST_CASE("GrainShifterProcessor::createProcessorDataLogFile captures default and modified shift_ratio", "[GrainShifterProcessor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/GRAIN_SHIFTER_PROCESSOR/OUTPUT/GrainShifterProcessor createProcessorDataLogFile captures default and modified shift_ratio")
                               .getChildFile (timestamp);

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
        processor.setDataLogRootDirectory (outputDir);
        processor.setDataLogOutputName ("default");
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
        processor.setDataLogRootDirectory (outputDir);
        processor.setDataLogOutputName ("modified");
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
