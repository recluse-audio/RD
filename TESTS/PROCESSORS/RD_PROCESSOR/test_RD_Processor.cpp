#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/BASE/RD_Processor.h"
#include "../../../SOURCE/BufferFiller.h"

TEST_CASE("RD_Processor createDataLogFile writes processor name and APVTS XML", "[RD_Processor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_Processor processor;

    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/RD_PROCESSOR/OUTPUT/RD_Processor createDataLogFile writes processor name and APVTS XML")
                               .getChildFile (timestamp);

    auto initialDir = outputDir.getChildFile ("initial");
    processor.createOutputDirectory (initialDir);
    processor.setOutputFile (initialDir);

    auto logFile = processor.createProcessorDataLogFile();

    REQUIRE(logFile.existsAsFile());

    auto json = juce::JSON::parse (logFile.loadFileAsString());
    REQUIRE(json["processorName"].toString() == processor.getName());

    auto apvtsXmlStr = json["apvts"].toString();
    REQUIRE(apvtsXmlStr.isNotEmpty());

    auto apvtsXml = juce::XmlDocument::parse (apvtsXmlStr);
    REQUIRE(apvtsXml != nullptr);

    auto* gainParam = apvtsXml->getChildByAttribute ("id", "gain");
    REQUIRE(gainParam != nullptr);
    REQUIRE(gainParam->getDoubleAttribute ("value") == Catch::Approx (1.0).margin (1e-5));

    processor.getAPVTS().getParameter ("gain")->setValueNotifyingHost (0.5f);

    juce::AudioBuffer<float> dummyBuffer (2, 512);
    juce::MidiBuffer dummyMidi;
    processor.processBlock (dummyBuffer, dummyMidi);

    auto afterGainDir = outputDir.getChildFile ("after-gain-change");
    processor.createOutputDirectory (afterGainDir);
    processor.setOutputFile (afterGainDir);

    auto logFile2 = processor.createProcessorDataLogFile();
    REQUIRE(logFile2.existsAsFile());

    auto json2     = juce::JSON::parse (logFile2.loadFileAsString());
    auto apvtsXml2 = juce::XmlDocument::parse (json2["apvts"].toString());
    REQUIRE(apvtsXml2 != nullptr);

    auto* gainParam2 = apvtsXml2->getChildByAttribute ("id", "gain");
    REQUIRE(gainParam2 != nullptr);
    REQUIRE(gainParam2->getDoubleAttribute ("value") == Catch::Approx (0.5).margin (1e-5));
}

TEST_CASE("RD_Processor caches sample rate and block size from prepareToPlay", "[RD_Processor]")
{
    TestUtils::SetupAndTeardown setup;
    RD_Processor processor;

    SECTION("Defaults before prepareToPlay")
    {
        REQUIRE(processor.getLastSampleRateFromPrepareToPlay() == 44100.0);
        REQUIRE(processor.getLastBlockSizeFromPrepareToPlay()  == 512);
    }

    SECTION("Values are cached after prepareToPlay")
    {
        processor.prepareToPlay(48000.0, 256);
        REQUIRE(processor.getLastSampleRateFromPrepareToPlay() == 48000.0);
        REQUIRE(processor.getLastBlockSizeFromPrepareToPlay()  == 256);
    }

    SECTION("Subsequent prepareToPlay calls overwrite cached values")
    {
        processor.prepareToPlay(48000.0, 256);
        processor.prepareToPlay(96000.0, 1024);
        REQUIRE(processor.getLastSampleRateFromPrepareToPlay() == 96000.0);
        REQUIRE(processor.getLastBlockSizeFromPrepareToPlay()  == 1024);
    }
}

TEST_CASE("RD_Processor tracks process sample count and resets on prepareToPlay", "[RD_Processor]")
{
    TestUtils::SetupAndTeardown setup;
    RD_Processor processor;

    SECTION("Default is zero before any prepareToPlay")
    {
        REQUIRE(processor.getProcessSampleCount() == 0);
    }

    SECTION("prepareToPlay resets count to zero")
    {
        processor.prepareToPlay (44100.0, 512);
        REQUIRE(processor.getProcessSampleCount() == 0);
    }

    SECTION("prepareToPlay clears a non-zero count")
    {
        // Simulate work done by subclasses bumping the count via the protected
        // member — exercised here through a derived test fixture.
        struct CountingProcessor : public RD_Processor
        {
            void bump (int n) { mProcessSampleCount += n; }
        };

        CountingProcessor counter;
        counter.bump (1024);
        REQUIRE(counter.getProcessSampleCount() == 1024);

        counter.prepareToPlay (48000.0, 256);
        REQUIRE(counter.getProcessSampleCount() == 0);
    }
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

    SECTION("Pre-processing flag produces preprocess_ file with index-equal samples")
    {
        auto sectionDir = outputDir.getChildFile ("preprocess");
        processor.createOutputDirectory (sectionDir);
        processor.setOutputFile (sectionDir);

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
        auto sectionDir = outputDir.getChildFile ("postprocess");
        processor.createOutputDirectory (sectionDir);
        processor.setOutputFile (sectionDir);

        auto logFile = processor.createProcessBlockDataLogFile (buffer, false);

        REQUIRE(logFile.existsAsFile());
        REQUIRE(logFile.getFileName().startsWith ("postprocess_"));
    }

    SECTION("Empty buffer writes header only")
    {
        auto sectionDir = outputDir.getChildFile ("empty-buffer");
        processor.createOutputDirectory (sectionDir);
        processor.setOutputFile (sectionDir);

        juce::AudioBuffer<float> empty (1, 0);
        auto logFile = processor.createProcessBlockDataLogFile (empty, true);

        REQUIRE(logFile.existsAsFile());
        auto lines = juce::StringArray::fromLines (logFile.loadFileAsString());
        REQUIRE(lines[0] == "ch0");
    }
}
