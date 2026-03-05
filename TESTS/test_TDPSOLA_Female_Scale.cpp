/**
 * test_TDPSOLA_Female_Scale.cpp
 * Test TDPSOLA_Processor with Female_Scale.wav golden file
 * Exports pitch marks and synth marks to CSV for analysis
 */

#include <catch2/catch_test_macros.hpp>
#include "../SOURCE/PROCESSORS/TDPSOLA/TDPSOLA_Processor.h"
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/BufferWriter.h"
#include "../SOURCE/AudioFileHelpers.h"
#include "TEST_UTILS/TestUtils.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>

// Structure to hold pitch mark data for export
struct PitchMarkSnapshot
{
    juce::int64 mark;           // Pitch mark position
    juce::int64 rangeStart;     // Analysis range start
    juce::int64 rangeEnd;       // Analysis range end
    float period;               // Detected period at this mark
};

// Structure to hold synth mark data for export
struct SynthMarkSnapshot
{
    juce::int64 synthMark;      // Synth mark position
    juce::int64 synthRangeStart; // Synth range start
    juce::int64 synthRangeEnd;   // Synth range end
    juce::int64 pitchMark;      // Source pitch mark position
    float shiftedPeriod;        // Shifted period (output period)
};

// Structure to hold complete processing history
struct TDPSOLA_ProcessingHistory
{
    float shiftRatio;
    int processedSamples;
    std::vector<PitchMarkSnapshot> pitchMarks;
    std::vector<SynthMarkSnapshot> synthMarks;
};

/**
 * Export pitch marks and synth marks to CSV files
 */
bool exportTDPSOLA_DataToCSV(const TDPSOLA_ProcessingHistory& history, const juce::String& outputPath)
{
    juce::String basePath = outputPath.upToLastOccurrenceOf(".", false, false);

    // Export pitch marks
    {
        juce::String csvPath = basePath + "_pitch_marks.csv";
        juce::File csvFile(csvPath);

        std::ofstream csvStream(csvFile.getFullPathName().toStdString());
        if (!csvStream.is_open())
            return false;

        csvStream << "mark,range_start,range_end,period\n";

        for (const auto& pm : history.pitchMarks)
        {
            csvStream << pm.mark << ","
                      << pm.rangeStart << ","
                      << pm.rangeEnd << ","
                      << pm.period << "\n";
        }

        csvStream.close();
    }

    // Export synth marks
    {
        juce::String csvPath = basePath + "_synth_marks.csv";
        juce::File csvFile(csvPath);

        std::ofstream csvStream(csvFile.getFullPathName().toStdString());
        if (!csvStream.is_open())
            return false;

        csvStream << "synth_mark,synth_range_start,synth_range_end,pitch_mark,shifted_period\n";

        for (const auto& sm : history.synthMarks)
        {
            csvStream << sm.synthMark << ","
                      << sm.synthRangeStart << ","
                      << sm.synthRangeEnd << ","
                      << sm.pitchMark << ","
                      << sm.shiftedPeriod << "\n";
        }

        csvStream.close();
    }

    // Export summary
    {
        juce::String summaryPath = basePath + "_summary.txt";
        juce::File summaryFile(summaryPath);

        std::ofstream summaryStream(summaryFile.getFullPathName().toStdString());
        if (!summaryStream.is_open())
            return false;

        summaryStream << "TDPSOLA_Processor Analysis Summary\n";
        summaryStream << "==================================================\n\n";
        summaryStream << "Pitch Shift Ratio: " << history.shiftRatio << "\n";
        summaryStream << "Processed Samples: " << history.processedSamples << " samples\n";
        summaryStream << "Number of Pitch Marks: " << history.pitchMarks.size() << "\n";
        summaryStream << "Number of Synth Marks: " << history.synthMarks.size() << "\n\n";

        if (!history.pitchMarks.empty())
        {
            double avgPeriod = 0.0;
            for (const auto& pm : history.pitchMarks)
                avgPeriod += pm.period;
            avgPeriod /= history.pitchMarks.size();
            summaryStream << "Average Detected Period: " << avgPeriod << " samples\n";
        }

        if (!history.synthMarks.empty())
        {
            double avgShiftedPeriod = 0.0;
            for (const auto& sm : history.synthMarks)
                avgShiftedPeriod += sm.shiftedPeriod;
            avgShiftedPeriod /= history.synthMarks.size();
            summaryStream << "Average Shifted Period: " << avgShiftedPeriod << " samples\n\n";
        }

        summaryStream << "Files generated:\n";
        summaryStream << "  - *_pitch_marks.csv: All detected pitch marks\n";
        summaryStream << "  - *_synth_marks.csv: All synthesis marks\n";
        summaryStream << "  - *.wav: Processed audio output\n";

        summaryStream.close();
    }

    return true;
}

TEST_CASE("TDPSOLA_Processor - Female_Scale.wav with pitch/synth mark export", "[TDPSOLA_Processor][female_scale]")
{
    TestUtils::SetupAndTeardown setup;

    // Create timestamp for unique output directory
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    juce::String timestamp(ss.str());

    // Test with different shift ratios
    SECTION("Shift ratio 1.0 (no pitch shift)")
    {
        float shiftRatio = 1.0f;

        juce::String outputDirName = juce::String("TDPSOLA_PROCESSOR_Female_Scale_") + juce::String(shiftRatio, 1) + "_" + timestamp;
        juce::File outputDir = juce::File::getCurrentWorkingDirectory()
                                .getChildFile("TESTS/OUTPUT")
                                .getChildFile(outputDirName);

        REQUIRE(outputDir.createDirectory());

        // Load input file
        juce::File currentDir = juce::File::getCurrentWorkingDirectory();
        juce::File inputFile = currentDir.getChildFile("TESTS/TEST_FILES/Female_Scale.wav");
        REQUIRE(inputFile.existsAsFile());

        juce::AudioBuffer<float> inputBuffer;
        bool loadSuccess = BufferFiller::loadFromWavFile(inputFile, inputBuffer);
        REQUIRE(loadSuccess);
        REQUIRE(inputBuffer.getNumSamples() > 0);

        double sampleRate = AudioFileHelpers::getFileSampleRate(inputFile);
        const int numChannels = inputBuffer.getNumChannels();

        // Process only first 5 seconds
        const int maxSamples = static_cast<int>(sampleRate * 5.0);
        const int numInputSamples = std::min(inputBuffer.getNumSamples(), maxSamples);

        // Initialize processor
        TDPSOLA_Processor processor;

        // Set shift ratio parameter (note: parameter ID is "shift_ratio" not "shiftRatio")
        auto& apvts = processor.getAPVTS();
        auto* shiftParam = apvts.getParameter("shift_ratio");
        REQUIRE(shiftParam != nullptr);
        shiftParam->setValueNotifyingHost(shiftParam->convertTo0to1(shiftRatio));

        const int blockSize = 512;
        processor.prepareToPlay(sampleRate, blockSize);

        // Prepare output buffer
        juce::AudioBuffer<float> outputBuffer;
        outputBuffer.setSize(numChannels, numInputSamples);
        outputBuffer.clear();

        juce::MidiBuffer midiBuffer;

        // Process audio block by block
        for (int startSample = 0; startSample < numInputSamples; startSample += blockSize)
        {
            const int samplesThisBlock = std::min(blockSize, numInputSamples - startSample);

            juce::AudioBuffer<float> blockBuffer(numChannels, samplesThisBlock);

            // Copy input to block buffer
            for (int ch = 0; ch < numChannels; ++ch)
            {
                blockBuffer.copyFrom(ch, 0, inputBuffer, ch, startSample, samplesThisBlock);
            }

            // Process block
            processor.processBlock(blockBuffer, midiBuffer);

            // Copy output
            for (int ch = 0; ch < numChannels; ++ch)
            {
                outputBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, samplesThisBlock);
            }
        }

        // Collect pitch marks and synth marks after processing
        TDPSOLA_ProcessingHistory history;
        history.shiftRatio = shiftRatio;
        history.processedSamples = numInputSamples;

        // Get pitch marks from PitchManager
        auto& pitchManager = processor.getPitchManager();
        const auto& pitchMarks = pitchManager.getPitchMarks();

        for (const auto& pm : pitchMarks)
        {
            if (!pm.isValid())
                continue;

            PitchMarkSnapshot snapshot;
            snapshot.mark = pm.mark;
            snapshot.rangeStart = pm.rangeStart;
            snapshot.rangeEnd = pm.rangeEnd;
            // Calculate period from range: period = rangeLength / 2
            snapshot.period = static_cast<float>(pm.getRangeLength()) / 2.0f;

            history.pitchMarks.push_back(snapshot);
        }

        // Get synth marks from PitchManager
        const auto& synthMarks = pitchManager.getSynthMarks();

        for (const auto& sm : synthMarks)
        {
            if (!sm.isValid())
                continue;

            SynthMarkSnapshot snapshot;
            snapshot.synthMark = sm.synthMark;
            snapshot.synthRangeStart = sm.synthRangeStart;
            snapshot.synthRangeEnd = sm.synthRangeEnd;
            snapshot.pitchMark = sm.pitchMark;
            // Calculate shifted period from synth range: shiftedPeriod = synthRangeLength / 2
            snapshot.shiftedPeriod = static_cast<float>(sm.getSynthRangeLength()) / 2.0f;

            history.synthMarks.push_back(snapshot);
        }

        // Write output WAV file
        juce::String outputFileName = juce::String("TDPSOLA_PROCESSOR_Female_Scale_") + juce::String(shiftRatio, 1) + "_" + timestamp + ".wav";
        juce::File outputFile = outputDir.getChildFile(outputFileName);
        BufferWriter::Result writeResult = BufferWriter::writeToWav(outputBuffer, outputFile, sampleRate, 24);
        REQUIRE(writeResult == BufferWriter::Result::kSuccess);
        REQUIRE(outputFile.existsAsFile());

        // Export CSV files
        REQUIRE(exportTDPSOLA_DataToCSV(history, outputFile.getFullPathName()));

        INFO("Processed " << numInputSamples << " samples");
        INFO("Captured " << history.pitchMarks.size() << " pitch marks");
        INFO("Captured " << history.synthMarks.size() << " synth marks");

        CHECK(history.pitchMarks.size() > 0);
        CHECK(history.synthMarks.size() > 0);
    }

    SECTION("Shift ratio 1.5 (pitch up by fifth)")
    {
        float shiftRatio = 1.5f;

        juce::String outputDirName = juce::String("TDPSOLA_PROCESSOR_Female_Scale_") + juce::String(shiftRatio, 1) + "_" + timestamp;
        juce::File outputDir = juce::File::getCurrentWorkingDirectory()
                                .getChildFile("TESTS/OUTPUT")
                                .getChildFile(outputDirName);

        REQUIRE(outputDir.createDirectory());

        juce::File currentDir = juce::File::getCurrentWorkingDirectory();
        juce::File inputFile = currentDir.getChildFile("TESTS/TEST_FILES/Female_Scale.wav");
        REQUIRE(inputFile.existsAsFile());

        juce::AudioBuffer<float> inputBuffer;
        bool loadSuccess = BufferFiller::loadFromWavFile(inputFile, inputBuffer);
        REQUIRE(loadSuccess);
        REQUIRE(inputBuffer.getNumSamples() > 0);

        double sampleRate = AudioFileHelpers::getFileSampleRate(inputFile);
        const int numChannels = inputBuffer.getNumChannels();

        const int maxSamples = static_cast<int>(sampleRate * 5.0);
        const int numInputSamples = std::min(inputBuffer.getNumSamples(), maxSamples);

        TDPSOLA_Processor processor;

        auto& apvts = processor.getAPVTS();
        auto* shiftParam = apvts.getParameter("shift_ratio");
        REQUIRE(shiftParam != nullptr);
        shiftParam->setValueNotifyingHost(shiftParam->convertTo0to1(shiftRatio));

        const int blockSize = 512;
        processor.prepareToPlay(sampleRate, blockSize);

        juce::AudioBuffer<float> outputBuffer;
        outputBuffer.setSize(numChannels, numInputSamples);
        outputBuffer.clear();

        juce::MidiBuffer midiBuffer;

        for (int startSample = 0; startSample < numInputSamples; startSample += blockSize)
        {
            const int samplesThisBlock = std::min(blockSize, numInputSamples - startSample);

            juce::AudioBuffer<float> blockBuffer(numChannels, samplesThisBlock);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                blockBuffer.copyFrom(ch, 0, inputBuffer, ch, startSample, samplesThisBlock);
            }

            processor.processBlock(blockBuffer, midiBuffer);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                outputBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, samplesThisBlock);
            }
        }

        TDPSOLA_ProcessingHistory history;
        history.shiftRatio = shiftRatio;
        history.processedSamples = numInputSamples;

        auto& pitchManager = processor.getPitchManager();
        const auto& pitchMarks = pitchManager.getPitchMarks();

        for (const auto& pm : pitchMarks)
        {
            if (!pm.isValid())
                continue;

            PitchMarkSnapshot snapshot;
            snapshot.mark = pm.mark;
            snapshot.rangeStart = pm.rangeStart;
            snapshot.rangeEnd = pm.rangeEnd;
            // Calculate period from range: period = rangeLength / 2
            snapshot.period = static_cast<float>(pm.getRangeLength()) / 2.0f;

            history.pitchMarks.push_back(snapshot);
        }

        const auto& synthMarks = pitchManager.getSynthMarks();

        for (const auto& sm : synthMarks)
        {
            if (!sm.isValid())
                continue;

            SynthMarkSnapshot snapshot;
            snapshot.synthMark = sm.synthMark;
            snapshot.synthRangeStart = sm.synthRangeStart;
            snapshot.synthRangeEnd = sm.synthRangeEnd;
            snapshot.pitchMark = sm.pitchMark;
            // Calculate shifted period from synth range: shiftedPeriod = synthRangeLength / 2
            snapshot.shiftedPeriod = static_cast<float>(sm.getSynthRangeLength()) / 2.0f;

            history.synthMarks.push_back(snapshot);
        }

        juce::String outputFileName = juce::String("TDPSOLA_PROCESSOR_Female_Scale_") + juce::String(shiftRatio, 1) + "_" + timestamp + ".wav";
        juce::File outputFile = outputDir.getChildFile(outputFileName);
        BufferWriter::Result writeResult = BufferWriter::writeToWav(outputBuffer, outputFile, sampleRate, 24);
        REQUIRE(writeResult == BufferWriter::Result::kSuccess);
        REQUIRE(outputFile.existsAsFile());

        REQUIRE(exportTDPSOLA_DataToCSV(history, outputFile.getFullPathName()));

        INFO("Processed " << numInputSamples << " samples");
        INFO("Captured " << history.pitchMarks.size() << " pitch marks");
        INFO("Captured " << history.synthMarks.size() << " synth marks");

        CHECK(history.pitchMarks.size() > 0);
        CHECK(history.synthMarks.size() > 0);
    }
}
