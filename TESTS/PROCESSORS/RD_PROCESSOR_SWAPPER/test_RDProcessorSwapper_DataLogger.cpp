#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/RD_ProcessorSwapper.h"
#include "../../../SOURCE/BufferFiller.h"

//========================================================
//===================== DATA LOGGING =====================
//========================================================
// Protocol for DataLogger inheriters:
//   1. Build timestamped outputDir under
//      TESTS/PROCESSORS/<PROCESSOR>/OUTPUT/<TEST CASE NAME>/<timestamp>.
//   2. Call processor.createOutputDirectory(outputDir) once per test case.
//   3. Per SECTION, create a sectionDir under outputDir, then
//      setOutputFile(sectionDir) so each section's logs are isolated.
//   4. Log pre-process buffer, run processBlock, log post-process buffer,
//      then log processor state. REQUIRE each returned juce::File exists.

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
    swapper.createOutputDirectory (outputDir);

    auto runGainSection = [&] (float gain, const juce::String& sectionName)
    {
        auto sectionDir = outputDir.getChildFile (sectionName);
        swapper.createOutputDirectory (sectionDir);
        swapper.setOutputFile (sectionDir);

        swapper.setGain (gain);

        juce::AudioBuffer<float> buffer (numChannels, numSamples);
        BufferFiller::fillWithAllOnes (buffer);

        auto preLog = swapper.createProcessBlockDataLogFile (buffer, true);
        REQUIRE(preLog.existsAsFile());

        juce::MidiBuffer midi;
        swapper.processBlock (buffer, midi);

        auto postLog = swapper.createProcessBlockDataLogFile (buffer, false);
        REQUIRE(postLog.existsAsFile());

        auto stateLog = swapper.createProcessorDataLogFile();
        REQUIRE(stateLog.existsAsFile());

        for (int ch = 0; ch < numChannels; ++ch)
            for (int s = 0; s < numSamples; ++s)
                REQUIRE(buffer.getSample (ch, s) == Catch::Approx (gain).margin (1e-6));
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
    swapper.createOutputDirectory (outputDir);

    auto runNoLogSection = [&] (float gain, const juce::String& sectionName)
    {
        auto sectionDir = outputDir.getChildFile (sectionName);
        swapper.createOutputDirectory (sectionDir);
        swapper.setOutputFile (sectionDir);

        swapper.setGlobalLoggingState (false);
        swapper.setGain (gain);

        juce::AudioBuffer<float> buffer (numChannels, numSamples);
        BufferFiller::fillWithAllOnes (buffer);

        juce::MidiBuffer midi;
        swapper.processBlock (buffer, midi);

        const juce::String preName  = "preprocess_"  + juce::String (numChannels) + "ch_" + juce::String (numSamples) + "smp.csv";
        const juce::String postName = "postprocess_" + juce::String (numChannels) + "ch_" + juce::String (numSamples) + "smp.csv";

        REQUIRE_FALSE(sectionDir.getChildFile (preName).existsAsFile());
        REQUIRE_FALSE(sectionDir.getChildFile (postName).existsAsFile());

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
    swapper.createOutputDirectory (outputDir);

    auto runApvtsSection = [&] (float gain, const juce::String& sectionName)
    {
        auto sectionDir = outputDir.getChildFile (sectionName);
        swapper.createOutputDirectory (sectionDir);
        swapper.setOutputFile (sectionDir);

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
