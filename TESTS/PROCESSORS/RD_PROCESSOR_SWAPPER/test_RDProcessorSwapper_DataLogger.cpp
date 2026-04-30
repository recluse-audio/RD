#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/RD_ProcessorSwapper.h"
#include "../../../SOURCE/BUFFER_FILLER/BufferFiller.h"

//========================================================
//===================== DATA LOGGING =====================
//========================================================
// Protocol for DataLogger inheriters:
//   1. Build timestamped outputDir under
//      TESTS/PROCESSORS/<PROCESSOR>/OUTPUT/<TEST CASE NAME>/<timestamp>.
//   2. Per SECTION, configure the logger then call startLogging():
//        processor.setDataLogRootDirectory(outputDir);
//        processor.setDataLogOutputName("<section name>");
//        processor.startLogging();
//   3. Run processBlock; appends happen automatically. Call stopLogging() when done.

TEST_CASE("RD_ProcessorSwapper applies gain and writes DataLogger output", "[RD_ProcessorSwapper][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;

    const double sampleRate  = 44100.0;
    const int    numChannels = 2;
    const int    numSamples  = 256;

    swapper.prepareToPlay (sampleRate, numSamples);
    swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGain);

    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/RD_PROCESSOR_SWAPPER/OUTPUT/RD_ProcessorSwapper applies gain and writes DataLogger output")
                               .getChildFile (timestamp);

    auto runGainSection = [&] (float gain, const juce::String& sectionName)
    {
        swapper.setDataLogRootDirectory (outputDir);
        swapper.setDataLogOutputName (sectionName);
        swapper.startLogging();

        swapper.setGain (gain);

        juce::MidiBuffer midi;
        const int numBlocks = 2;

        for (int b = 0; b < numBlocks; ++b)
        {
            juce::AudioBuffer<float> buffer (numChannels, numSamples);
            BufferFiller::fillWithAllOnes (buffer);
            swapper.processBlock (buffer, midi);

            for (int ch = 0; ch < numChannels; ++ch)
                for (int s = 0; s < numSamples; ++s)
                    REQUIRE (buffer.getSample (ch, s) == Catch::Approx (gain).margin (1e-6));
        }

        auto sectionDir = outputDir.getChildFile (sectionName);
        REQUIRE (sectionDir.getChildFile ("input_samples.csv") .existsAsFile());
        REQUIRE (sectionDir.getChildFile ("output_samples.csv").existsAsFile());

        auto stateLog = swapper.createProcessorDataLogFile();
        REQUIRE (stateLog.existsAsFile());

        swapper.stopLogging();
    };

    SECTION("Gain 1.0 leaves all-ones buffer unchanged")
    {
        runGainSection (1.0f, "gain-1.0");
    }

    SECTION("Gain 0.5 halves all-ones buffer")
    {
        runGainSection (0.5f, "gain-0.5");
    }

    swapper.releaseResources();
}

TEST_CASE("RD_ProcessorSwapper writes no CSV when global logging is disabled", "[RD_ProcessorSwapper][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;

    const double sampleRate  = 44100.0;
    const int    numChannels = 2;
    const int    numSamples  = 256;

    swapper.prepareToPlay (sampleRate, numSamples);
    swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGain);

    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/RD_PROCESSOR_SWAPPER/OUTPUT/RD_ProcessorSwapper writes no CSV when global logging is disabled")
                               .getChildFile (timestamp);

    auto runNoLogSection = [&] (float gain, const juce::String& sectionName)
    {
        swapper.setDataLogRootDirectory (outputDir);
        swapper.setDataLogOutputName (sectionName);
        auto sectionDir = swapper.getDataLogOutputDirectory();

        swapper.setIsLogging (false);
        swapper.setGain (gain);

        juce::AudioBuffer<float> buffer (numChannels, numSamples);
        BufferFiller::fillWithAllOnes (buffer);

        juce::MidiBuffer midi;
        swapper.processBlock (buffer, midi);

        REQUIRE_FALSE (sectionDir.getChildFile ("input_samples.csv") .existsAsFile());
        REQUIRE_FALSE (sectionDir.getChildFile ("output_samples.csv").existsAsFile());

        for (int ch = 0; ch < numChannels; ++ch)
            for (int s = 0; s < numSamples; ++s)
                REQUIRE(buffer.getSample (ch, s) == Catch::Approx (gain).margin (1e-6));
    };

    SECTION("Gain 1.0 with logging off writes no CSV")
    {
        runNoLogSection (1.0f, "gain-1.0-nolog");
    }

    SECTION("Gain 0.5 with logging off writes no CSV")
    {
        runNoLogSection (0.5f, "gain-0.5-nolog");
    }

    swapper.releaseResources();
}

TEST_CASE("RD_ProcessorSwapper writes APVTS gain value into processor state log", "[RD_ProcessorSwapper][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;

    const double sampleRate  = 44100.0;
    const int    numSamples  = 256;

    swapper.prepareToPlay (sampleRate, numSamples);

    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/RD_PROCESSOR_SWAPPER/OUTPUT/RD_ProcessorSwapper writes APVTS gain value into processor state log")
                               .getChildFile (timestamp);

    auto runApvtsSection = [&] (float gain, const juce::String& sectionName)
    {
        swapper.setDataLogRootDirectory (outputDir);
        swapper.setDataLogOutputName (sectionName);
        swapper.getDataLogOutputDirectory().createDirectory();

        auto* param = swapper.getAPVTS().getParameter ("gain");
        REQUIRE(param != nullptr);
        param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (gain));

        auto stateLog = swapper.createProcessorDataLogFile();
        REQUIRE(stateLog.existsAsFile());
        REQUIRE(stateLog.getFileExtension() == ".xml");

        auto xml = juce::XmlDocument::parse (stateLog);
        REQUIRE(xml != nullptr);
        REQUIRE(xml->getStringAttribute ("processorName") == swapper.getName());

        auto* gainElem = xml->getChildByAttribute ("id", "gain");
        REQUIRE(gainElem != nullptr);
        REQUIRE(gainElem->hasAttribute ("value"));
        REQUIRE(gainElem->getDoubleAttribute ("value") == Catch::Approx (gain).margin (1e-6));
    };

    SECTION("APVTS gain 1.0 round-trips through state log")
    {
        runApvtsSection (1.0f, "apvts-gain-1.0");
    }

    SECTION("APVTS gain 0.5 round-trips through state log")
    {
        runApvtsSection (0.5f, "apvts-gain-0.5");
    }

    swapper.releaseResources();
}
