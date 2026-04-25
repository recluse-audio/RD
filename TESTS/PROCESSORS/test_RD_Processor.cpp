#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../TEST_UTILS/TestUtils.h"
#include "../../SOURCE/PROCESSORS/BASE/RD_Processor.h"

TEST_CASE("RD_Processor createDataLogFile writes processor name and APVTS XML", "[RD_Processor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_Processor processor;

    juce::File outputDir ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT");
    processor.setOutputFile (outputDir);

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

TEST_CASE("RD_Processor createProcessBlockDataLogFile writes audio buffer as CSV", "[RD_Processor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_Processor processor;

    juce::File outputDir ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/DATA_LOGGER/OUTPUT");
    processor.setOutputFile (outputDir);

    const int numChannels = 2;
    const int numSamples  = 8;
    juce::AudioBuffer<float> buffer (numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
        for (int s = 0; s < numSamples; ++s)
            buffer.setSample (ch, s, static_cast<float> (ch) + static_cast<float> (s) * 0.1f);

    SECTION("Pre-processing flag produces preprocess_ file")
    {
        auto logFile = processor.createProcessBlockDataLogFile (buffer, true);

        REQUIRE(logFile.existsAsFile());
        REQUIRE(logFile.getFileName().startsWith ("preprocess_"));
        REQUIRE(logFile.getFileExtension() == ".csv");

        auto lines = juce::StringArray::fromLines (logFile.loadFileAsString());
        // header + numSamples rows + possible trailing empty line
        REQUIRE(lines[0] == "ch0,ch1");
        REQUIRE(lines.size() >= numSamples + 1);

        // Spot-check first data row: ch0=0.0, ch1=1.0
        auto firstRow = juce::StringArray::fromTokens (lines[1], ",", "");
        REQUIRE(firstRow.size() == numChannels);
        REQUIRE(firstRow[0].getFloatValue() == Catch::Approx (0.0f).margin (1e-5));
        REQUIRE(firstRow[1].getFloatValue() == Catch::Approx (1.0f).margin (1e-5));

        // Last data row: ch0 = 0.7, ch1 = 1.7
        auto lastRow = juce::StringArray::fromTokens (lines[numSamples], ",", "");
        REQUIRE(lastRow[0].getFloatValue() == Catch::Approx (0.7f).margin (1e-5));
        REQUIRE(lastRow[1].getFloatValue() == Catch::Approx (1.7f).margin (1e-5));
    }

    SECTION("Post-processing flag produces postprocess_ file")
    {
        auto logFile = processor.createProcessBlockDataLogFile (buffer, false);

        REQUIRE(logFile.existsAsFile());
        REQUIRE(logFile.getFileName().startsWith ("postprocess_"));
    }

    SECTION("Empty buffer writes header only")
    {
        juce::AudioBuffer<float> empty (1, 0);
        auto logFile = processor.createProcessBlockDataLogFile (empty, true);

        REQUIRE(logFile.existsAsFile());
        auto lines = juce::StringArray::fromLines (logFile.loadFileAsString());
        REQUIRE(lines[0] == "ch0");
    }
}
