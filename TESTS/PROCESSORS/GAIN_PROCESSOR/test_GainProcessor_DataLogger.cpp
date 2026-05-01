#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/GAIN/GainProcessor.h"
#include "../../../SOURCE/BUFFER_FILLER/BufferFiller.h"

//========================================================
//===================== DATA LOGGING =====================
//========================================================
// Protocol for DataLogger inheriters:
//   Path layout:
//     <OUTPUT_BASE>/<TEST_NAME>[/<SECTION_NAME>]/TEST_CASE_ROOT_DIR/DATA_LOG_OUTPUT_DIR
//   where TEST_CASE_ROOT_DIR is the literal root passed to setDataLogRootDirectory
//   and DATA_LOG_OUTPUT_DIR is the literal name passed to
//   setDataLogOutputName. Drop the SECTION_NAME segment for tests with no SECTION.
//   1. Per SECTION (or once per case), configure the logger then call startLogging():
//        processor.setDataLogRootDirectory (testDir/[sectionName]/"TEST_CASE_ROOT_DIR");
//        processor.setDataLogOutputName ("DATA_LOG_OUTPUT_DIR");
//        processor.startLogging();
//   2. Run processBlock one or more times. Each call appends 2 + numChannels
//      rows to input_samples.csv (pre) and output_samples.csv (post):
//      [global indices, local indices, ch0 values, ch1 values, ...].
//      After N blocks, each file has (2 + numChannels) * N rows.
//      Call stopLogging() when done.

TEST_CASE("GainProcessor applies gain and writes DataLogger output", "[GainProcessor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    GainProcessor processor;

    juce::File testDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/GAIN_PROCESSOR/OUTPUT/GainProcessor applies gain and writes DataLogger output");

    const int numChannels = 2;
    const int numSamples  = 256;

    auto runGainSection = [&] (float gain, const juce::String& sectionName)
    {
        auto rootDir    = testDir.getChildFile (sectionName).getChildFile ("TEST_CASE_ROOT_DIR");
        auto outputName = juce::String ("DATA_LOG_OUTPUT_DIR");

        processor.setDataLogRootDirectory (rootDir);
        processor.setDataLogOutputName (outputName);
        processor.startLogging();

        processor.setGain (gain);

        const int numBlocks = 2;
        juce::MidiBuffer midi;

        for (int b = 0; b < numBlocks; ++b)
        {
            juce::AudioBuffer<float> buffer (numChannels, numSamples);
            BufferFiller::fillWithAllOnes (buffer);
            processor.processBlock (buffer, midi);

            for (int ch = 0; ch < numChannels; ++ch)
                for (int s = 0; s < numSamples; ++s)
                    REQUIRE (buffer.getSample (ch, s) == Catch::Approx (gain).margin (1e-6));
        }

        auto outputDir = rootDir.getChildFile (outputName);
        auto inFile  = outputDir.getChildFile ("input_samples.csv");
        auto outFile = outputDir.getChildFile ("output_samples.csv");
        REQUIRE (inFile .existsAsFile());
        REQUIRE (outFile.existsAsFile());

        auto countLines = [] (const juce::File& f)
        {
            return juce::StringArray::fromLines (f.loadFileAsString().trimEnd()).size();
        };
        const int rowsPerBlock = 2 + numChannels;
        REQUIRE (countLines (inFile)  == rowsPerBlock * numBlocks);
        REQUIRE (countLines (outFile) == rowsPerBlock * numBlocks);

        auto stateLog = processor.createProcessorDataLogFile();
        REQUIRE (stateLog.existsAsFile());

        processor.stopLogging();
    };

    SECTION("Gain 1.0 leaves all-ones buffer unchanged")
    {
        runGainSection (1.0f, "gain-1.0");
    }

    SECTION("Gain 0.5 halves all-ones buffer")
    {
        runGainSection (0.5f, "gain-0.5");
    }
}

TEST_CASE("GainProcessor logs raw input and gain-scaled output rows across consecutive processBlock calls", "[GainProcessor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File rootDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/GAIN_PROCESSOR/OUTPUT/GainProcessor logs raw input and gain-scaled output rows")
                             .getChildFile ("TEST_CASE_ROOT_DIR");

    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    GainProcessor processor;
    processor.setDataLogRootDirectory (rootDir);
    processor.setDataLogOutputName    (outputName);
    processor.startLogging();

    const float gain        = 0.5f;
    const int   numChannels = 2;
    const int   blockSize   = 64;
    const int   numBlocks   = 3;

    processor.setGain (gain);

    juce::AudioBuffer<float> buffer (numChannels, blockSize);
    juce::MidiBuffer midi;

    for (int b = 0; b < numBlocks; ++b)
    {
        BufferFiller::fillWithAllOnes (buffer);
        processor.processBlock (buffer, midi);
    }

    auto outputDir = processor.getDataLogOutputDirectory();

    auto buildIndicesRow = [] (juce::int64 startIndex, int n)
    {
        juce::String row;
        for (int s = 0; s < n; ++s)
        {
            if (s > 0) row << ",";
            row << juce::String (startIndex + s);
        }
        return row;
    };

    auto buildConstantValuesRow = [] (int n, float value)
    {
        juce::String row;
        for (int s = 0; s < n; ++s)
        {
            if (s > 0) row << ",";
            row << juce::String (value, 8);
        }
        return row;
    };

    auto inFile  = outputDir.getChildFile ("input_samples.csv");
    auto outFile = outputDir.getChildFile ("output_samples.csv");
    REQUIRE (inFile .existsAsFile());
    REQUIRE (outFile.existsAsFile());

    auto inLines  = juce::StringArray::fromLines (inFile .loadFileAsString().trimEnd());
    auto outLines = juce::StringArray::fromLines (outFile.loadFileAsString().trimEnd());

    const int rowsPerBlock = 2 + numChannels;
    REQUIRE (inLines .size() == rowsPerBlock * numBlocks);
    REQUIRE (outLines.size() == rowsPerBlock * numBlocks);

    for (int b = 0; b < numBlocks; ++b)
    {
        const juce::int64  globalStart  = static_cast<juce::int64> (b) * blockSize;
        const juce::String globalRow    = buildIndicesRow        (globalStart, blockSize);
        const juce::String localRow     = buildIndicesRow        (0,           blockSize);
        const juce::String inChannelRow = buildConstantValuesRow (blockSize, 1.0f);
        const juce::String outChannelRow= buildConstantValuesRow (blockSize, gain);

        const int base = b * rowsPerBlock;

        REQUIRE (inLines [base + 0] == globalRow);
        REQUIRE (inLines [base + 1] == localRow);
        REQUIRE (inLines [base + 2] == inChannelRow);
        REQUIRE (inLines [base + 3] == inChannelRow);

        REQUIRE (outLines[base + 0] == globalRow);
        REQUIRE (outLines[base + 1] == localRow);
        REQUIRE (outLines[base + 2] == outChannelRow);
        REQUIRE (outLines[base + 3] == outChannelRow);
    }

    processor.stopLogging();
}

TEST_CASE("GainProcessor rotates samples CSV at max size byte limit", "[GainProcessor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File rootDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/GAIN_PROCESSOR/OUTPUT/GainProcessor rotates samples CSV at max size byte limit")
                             .getChildFile ("TEST_CASE_ROOT_DIR");
    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    GainProcessor processor;
    processor.setDataLogRootDirectory (rootDir);
    processor.setDataLogOutputName    (outputName);

    const size_t maxBytes   = 512;
    const int    numChannels = 2;
    const int    blockSize   = 64;
    const int    numBlocks   = 12;

    processor.setMaxCsvSizeBytes (maxBytes);
    processor.startLogging();
    processor.setGain (0.5f);

    juce::AudioBuffer<float> buffer (numChannels, blockSize);
    juce::MidiBuffer midi;

    for (int b = 0; b < numBlocks; ++b)
    {
        BufferFiller::fillWithAllOnes (buffer);
        processor.processBlock (buffer, midi);
    }

    auto outputDir = processor.getDataLogOutputDirectory();

    auto collectRotated = [&] (const juce::String& stem)
    {
        std::vector<juce::File> files;
        files.push_back (outputDir.getChildFile (stem + ".csv"));
        for (int i = 1;; ++i)
        {
            auto f = outputDir.getChildFile (stem + "_" + juce::String (i) + ".csv");
            if (! f.existsAsFile()) break;
            files.push_back (f);
        }
        return files;
    };

    auto countLines = [] (const juce::File& f)
    {
        return juce::StringArray::fromLines (f.loadFileAsString().trimEnd()).size();
    };

    const int rowsPerBlock = 2 + numChannels;

    for (const auto& stem : { juce::String ("input_samples"), juce::String ("output_samples") })
    {
        auto files = collectRotated (stem);
        REQUIRE (files.size() >= 3);
        REQUIRE (files[0].existsAsFile());

        int totalRows = 0;
        for (size_t i = 0; i < files.size(); ++i)
        {
            REQUIRE (files[i].existsAsFile());
            totalRows += countLines (files[i]);
            if (i + 1 < files.size())
                REQUIRE (static_cast<size_t> (files[i].getSize()) >= maxBytes);
        }
        REQUIRE (totalRows == rowsPerBlock * numBlocks);
    }

    processor.stopLogging();

    processor.startLogging();
    REQUIRE_FALSE (outputDir.getChildFile ("input_samples_1.csv").existsAsFile());
    REQUIRE_FALSE (outputDir.getChildFile ("output_samples_1.csv").existsAsFile());
    processor.stopLogging();
}

TEST_CASE("GainProcessor prepareToPlay logs sampleRate and maxBlockSize", "[GainProcessor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File rootDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/GAIN_PROCESSOR/OUTPUT/GainProcessor prepareToPlay logs sampleRate and maxBlockSize")
                             .getChildFile ("TEST_CASE_ROOT_DIR");

    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    GainProcessor processor;
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

TEST_CASE("GainProcessor::createProcessorDataLogFile captures default and modified gain", "[GainProcessor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File testDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/GAIN_PROCESSOR/OUTPUT/GainProcessor createProcessorDataLogFile captures default and modified gain");

    auto makeSectionRoot = [&] (const juce::String& sectionName)
    {
        return testDir.getChildFile (sectionName).getChildFile ("TEST_CASE_ROOT_DIR");
    };
    auto makeOutputName = [] ()
    {
        return juce::String ("DATA_LOG_OUTPUT_DIR");
    };

    auto readGainFromXml = [] (const juce::File& file) -> float
    {
        auto xml = juce::XmlDocument::parse (file);
        REQUIRE (xml != nullptr);
        for (auto* child : xml->getChildIterator())
        {
            if (child->hasTagName ("PARAM") && child->getStringAttribute ("id") == "gain")
                return static_cast<float> (child->getDoubleAttribute ("value"));
        }
        FAIL ("gain PARAM not found in processor_state.xml");
        return 0.0f;
    };

    SECTION("Default gain = 1.0 written to processor_state.xml")
    {
        GainProcessor processor;
        processor.setDataLogRootDirectory (makeSectionRoot ("default"));
        processor.setDataLogOutputName (makeOutputName());
        processor.startLogging();

        auto stateFile = processor.createProcessorDataLogFile();
        REQUIRE (stateFile.existsAsFile());
        REQUIRE (stateFile == processor.getDataLogOutputDirectory().getChildFile ("processor_state.xml"));

        auto xml = juce::XmlDocument::parse (stateFile);
        REQUIRE (xml != nullptr);
        REQUIRE (xml->getStringAttribute ("processorName") == processor.getName());

        REQUIRE (readGainFromXml (stateFile) == Catch::Approx (1.0f));

        processor.stopLogging();
    }

    SECTION("Modified gain = 0.25 written to processor_state.xml")
    {
        GainProcessor processor;
        processor.setDataLogRootDirectory (makeSectionRoot ("modified"));
        processor.setDataLogOutputName (makeOutputName());
        processor.startLogging();

        auto* gainParam = processor.getAPVTS().getParameter ("gain");
        REQUIRE (gainParam != nullptr);
        gainParam->setValueNotifyingHost (0.25f);

        auto stateFile = processor.createProcessorDataLogFile();
        REQUIRE (stateFile.existsAsFile());

        REQUIRE (readGainFromXml (stateFile) == Catch::Approx (0.25f));

        processor.stopLogging();
    }
}
