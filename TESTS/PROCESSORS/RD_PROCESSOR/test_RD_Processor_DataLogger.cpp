#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/BASE/RD_Processor.h"
#include "../../../SOURCE/BUFFER_FILLER/BufferFiller.h"

//========================================================
//===================== DATA LOGGING =====================
//========================================================
// Verifies RD_Processor's inherited DataLogger base-class behavior.
//
// Per-block CSV schema (input_samples.csv / output_samples.csv):
//   Row 0: global sample indices (mProcessSampleCount + s) for s in [0, blockSize)
//   Row 1: local  sample indices (s)                       for s in [0, blockSize)
//   Row 2: ch0 sample values
//   Row 3: ch1 sample values
// Total rows per file = 4 * numBlocks (for stereo).

TEST_CASE("RD_Processor processBlock writes input/output sample CSVs to composed output directory", "[RD_Processor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("RD_Processor processBlock writes input_output sample CSVs")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");

    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    RD_Processor processor;
    processor.setDataLogRootDirectory (rootDir);
    processor.setDataLogOutputName    (outputName);
    processor.startLogging();

    const int numChannels = 2;
    const int numSamples  = 256;
    juce::AudioBuffer<float> buffer (numChannels, numSamples);
    BufferFiller::fillWithAllOnes (buffer);

    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);

    auto outputDir = processor.getDataLogOutputDirectory();
    REQUIRE (outputDir == rootDir.getChildFile (outputName));
    REQUIRE (outputDir.isDirectory());

    auto inFile  = outputDir.getChildFile ("input_samples.csv");
    auto outFile = outputDir.getChildFile ("output_samples.csv");
    REQUIRE (inFile .existsAsFile());
    REQUIRE (outFile.existsAsFile());

    auto countLines = [] (const juce::File& f)
    {
        return juce::StringArray::fromLines (f.loadFileAsString().trimEnd()).size();
    };

    const int rowsPerBlock = 2 + numChannels;
    REQUIRE (countLines (inFile)  == rowsPerBlock);
    REQUIRE (countLines (outFile) == rowsPerBlock);

    processor.stopLogging();
}

TEST_CASE("RD_Processor prepareToPlay logs sampleRate and maxBlockSize", "[RD_Processor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("RD_Processor prepareToPlay logs sampleRate and maxBlockSize")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");

    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    RD_Processor processor;
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

TEST_CASE("RD_Processor::createProcessorDataLogFile writes processor_state.xml inside output directory", "[RD_Processor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("RD_Processor createProcessorDataLogFile writes processor_state xml")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");

    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    RD_Processor processor;
    processor.setDataLogRootDirectory (rootDir);
    processor.setDataLogOutputName    (outputName);
    processor.startLogging();

    juce::AudioBuffer<float> buffer (2, 64);
    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);

    auto stateFile = processor.createProcessorDataLogFile();

    auto outputDir = processor.getDataLogOutputDirectory();
    REQUIRE (stateFile.existsAsFile());
    REQUIRE (stateFile == outputDir.getChildFile ("processor_state.xml"));
    REQUIRE (stateFile.getParentDirectory() == outputDir);

    auto xml = juce::XmlDocument::parse (stateFile);
    REQUIRE (xml != nullptr);
    REQUIRE (xml->getStringAttribute ("processorName") == processor.getName());

    processor.stopLogging();
}

TEST_CASE("RD_Processor lifecycle log calls also write processor_state.xml", "[RD_Processor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("RD_Processor lifecycle log calls write processor_state xml")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");

    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    RD_Processor processor;
    processor.setDataLogRootDirectory (rootDir);
    processor.setDataLogOutputName    (outputName);

    auto outputDir = processor.getDataLogOutputDirectory();
    outputDir.deleteRecursively();

    processor.startLogging();

    SECTION("prepareToPlay writes processor_state.xml in prepare_to_play subdir")
    {
        auto stateFile = outputDir.getChildFile ("prepare_to_play").getChildFile ("processor_state.xml");
        REQUIRE_FALSE (stateFile.existsAsFile());
        processor.prepareToPlay (44100.0, 256);
        REQUIRE (stateFile.existsAsFile());

        auto xml = juce::XmlDocument::parse (stateFile);
        REQUIRE (xml != nullptr);
        REQUIRE (xml->getStringAttribute ("processorName") == processor.getName());
    }

    SECTION("no processor_state.xml when logging disabled")
    {
        processor.stopLogging();
        processor.prepareToPlay (44100.0, 256);

        juce::AudioBuffer<float> buffer (2, 64);
        juce::MidiBuffer midi;
        processor.processBlock (buffer, midi);

        REQUIRE_FALSE (outputDir.getChildFile ("prepare_to_play").getChildFile ("processor_state.xml").existsAsFile());
    }

    processor.stopLogging();
}

TEST_CASE("RD_Processor logs global + local indices and per-channel samples across consecutive processBlock calls", "[RD_Processor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("RD_Processor logs global_local indices and per-channel samples across processBlock calls")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");

    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    RD_Processor processor;
    processor.setDataLogRootDirectory (rootDir);
    processor.setDataLogOutputName    (outputName);
    processor.startLogging();

    const int numChannels = 2;
    const int blockSize   = 64;
    const int numBlocks   = 3;

    juce::AudioBuffer<float> buffer (numChannels, blockSize);
    juce::MidiBuffer midi;

    for (int b = 0; b < numBlocks; ++b)
    {
        BufferFiller::fillIncremental (buffer);
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

    auto buildIncrementalValuesRow = [] (int n)
    {
        juce::String row;
        for (int s = 0; s < n; ++s)
        {
            if (s > 0) row << ",";
            row << juce::String (static_cast<float> (s), 8);
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
        const juce::int64  globalStart = static_cast<juce::int64> (b) * blockSize;
        const juce::String globalRow   = buildIndicesRow (globalStart, blockSize);
        const juce::String localRow    = buildIndicesRow (0,           blockSize);
        const juce::String channelRow  = buildIncrementalValuesRow (blockSize);

        const int base = b * rowsPerBlock;

        REQUIRE (inLines [base + 0] == globalRow);
        REQUIRE (inLines [base + 1] == localRow);
        REQUIRE (inLines [base + 2] == channelRow);
        REQUIRE (inLines [base + 3] == channelRow);

        REQUIRE (outLines[base + 0] == globalRow);
        REQUIRE (outLines[base + 1] == localRow);
        REQUIRE (outLines[base + 2] == channelRow);
        REQUIRE (outLines[base + 3] == channelRow);
    }

    processor.stopLogging();
}
