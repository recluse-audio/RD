/**
 * MarkWriter.cpp
 * Created by Ryan Devens
 */

#include "MarkWriter.h"
#include <fstream>

//=======================================
juce::String MarkWriter::getCurrentTimestamp()
{
    auto now = juce::Time::getCurrentTime();
    return now.formatted("%Y%m%d_%H%M%S");
}

//=======================================
bool MarkWriter::createDirectoryIfNeeded(const juce::String& path)
{
    juce::File directory(path);

    if (!directory.exists())
    {
        juce::Result result = directory.createDirectory();
        if (result.failed())
        {
            DBG("Failed to create directory: " + path + " - " + result.getErrorMessage());
            return false;
        }
    }

    return true;
}

//=======================================
bool MarkWriter::writeMarksToCSV(const std::vector<PitchMark>& pitchMarks,
                                  const std::vector<SynthMark>& synthMarks,
                                  const juce::String& outputDir)
{
    // Get timestamp for directory and file names
    juce::String timestamp = getCurrentTimestamp();

    // Create directory structure: outputDir/MARKS/MARKS_<timestamp>/
    juce::String marksDir = outputDir + "/MARKS";
    juce::String timestampedDir = marksDir + "/MARKS_" + timestamp;

    if (!createDirectoryIfNeeded(marksDir))
        return false;

    if (!createDirectoryIfNeeded(timestampedDir))
        return false;

    // Create CSV file: marks_<timestamp>.csv
    juce::String filename = timestampedDir + "/marks_" + timestamp + ".csv";
    juce::File csvFile(filename);

    // Open file for writing
    std::ofstream file(csvFile.getFullPathName().toStdString());
    if (!file.is_open())
    {
        DBG("Failed to open file for writing: " + filename);
        return false;
    }

    // Write CSV header
    file << "Type,Index,MarkPosition,RangeStart,RangeEnd,RangeLength,";
    file << "PitchMarkRef,PitchRangeStart,PitchRangeEnd,";
    file << "SynthMark,SynthRangeStart,SynthRangeEnd,SynthRangeLength\n";

    // Write pitch marks
    for (size_t i = 0; i < pitchMarks.size(); ++i)
    {
        const auto& pm = pitchMarks[i];

        file << "PitchMark," << i << ",";
        file << pm.mark << "," << pm.rangeStart << "," << pm.rangeEnd << ",";
        file << pm.getRangeLength() << ",";

        // Empty columns for synth-specific data
        file << ",,,,,,\n";
    }

    // Write synth marks
    for (size_t i = 0; i < synthMarks.size(); ++i)
    {
        const auto& sm = synthMarks[i];

        file << "SynthMark," << i << ",";

        // Mark position and range (for synth mark, use synth position)
        file << sm.synthMark << ",";
        file << sm.synthRangeStart << "," << sm.synthRangeEnd << ",";
        file << sm.getSynthRangeLength() << ",";

        // Pitch mark reference data
        file << sm.pitchMark << "," << sm.pitchRangeStart << "," << sm.pitchRangeEnd << ",";

        // Synth mark specific data
        file << sm.synthMark << "," << sm.synthRangeStart << "," << sm.synthRangeEnd << ",";
        file << sm.getSynthRangeLength() << "\n";
    }

    file.close();

    DBG("Successfully wrote marks to: " + filename);
    DBG("  Pitch marks: " + juce::String(pitchMarks.size()));
    DBG("  Synth marks: " + juce::String(synthMarks.size()));

    return true;
}
