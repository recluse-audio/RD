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

TEST_CASE("GrainShifterProcessor child loggers (PitchManager, Granulator) write range CSVs", "[GrainShifterProcessor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("GrainShifterProcessor child range CSVs")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");

    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    GrainShifterProcessor processor;
    processor.setDataLogRootDirectory (rootDir);
    processor.setDataLogOutputName    (outputName);
    processor.startLogging();
    // Block-level CSVs aren't needed here and bloat the output dir; disable.
    processor.setIsBlockLogging (false);

    const double sampleRate = 48000.0;
    const int    blockSize  = 1024;
    const int    numChannels = 2;

    processor.setPlayConfigDetails (numChannels, numChannels, sampleRate, blockSize);
    processor.prepareToPlay (sampleRate, blockSize);

    // Run enough blocks to exceed several detection windows (default 2048).
    juce::AudioBuffer<float> buffer (numChannels, blockSize);
    juce::MidiBuffer midi;

    const int numBlocks = 8;
    for (int b = 0; b < numBlocks; ++b)
    {
        // Fill with a 220 Hz sine so pitch detection has something coherent.
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int s = 0; s < blockSize; ++s)
            {
                const double t = static_cast<double> (b * blockSize + s) / sampleRate;
                data[s] = 0.5f * std::sin (juce::MathConstants<double>::twoPi * 220.0 * t);
            }
        }
        processor.processBlock (buffer, midi);
    }

    auto outputDir   = processor.getDataLogOutputDirectory();
    auto pitchCsv    = outputDir.getChildFile ("pitch_manager").getChildFile ("detect_log.csv");
    auto granCsv     = outputDir.getChildFile ("granulator").getChildFile ("synthesis_grains.csv");

    REQUIRE (pitchCsv.existsAsFile());

    auto pitchLines = juce::StringArray::fromLines (pitchCsv.loadFileAsString().trimEnd());
    REQUIRE (pitchLines.size() >= 2);
    REQUIRE (pitchLines[0] == "detect_call_id,read_start_abs_process_count,read_end_abs_process_count,window_size,period");

    // Verify first data row's read_start/read_end span equals window_size.
    auto firstRow = juce::StringArray::fromTokens (pitchLines[1], ",", "");
    REQUIRE (firstRow.size() == 5);
    const auto startAbs   = firstRow[1].getLargeIntValue();
    const auto endAbs     = firstRow[2].getLargeIntValue();
    const auto windowSize = firstRow[3].getIntValue();
    REQUIRE (endAbs - startAbs == windowSize);

    // Granulator only logs when synth marks were present and the granulation
    // counter elapsed; over 8 blocks of clean sine that should fire at least once.
    if (granCsv.existsAsFile())
    {
        auto granLines = juce::StringArray::fromLines (granCsv.loadFileAsString().trimEnd());
        REQUIRE (granLines.size() >= 2);
        REQUIRE (granLines[0] == "source_analysis_id,source_start,source_center,source_end,grain_id,start_sample,center_sample,end_sample,source_period,synthesis_period,duration_samples,window_alpha");
    }

    processor.stopLogging();
}
