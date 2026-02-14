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

    // ========== Write Pitch Marks CSV ==========
    juce::String pitchFilename = timestampedDir + "/pitchmarks_" + timestamp + ".csv";
    juce::File pitchCsvFile(pitchFilename);

    std::ofstream pitchFile(pitchCsvFile.getFullPathName().toStdString());
    if (!pitchFile.is_open())
    {
        DBG("Failed to open file for writing: " + pitchFilename);
        return false;
    }

    // Write pitch marks CSV header
    pitchFile << "Index,RangeStart,MarkPosition,RangeEnd,RangeLength\n";

    // Write pitch marks
    for (size_t i = 0; i < pitchMarks.size(); ++i)
    {
        const auto& pm = pitchMarks[i];
        pitchFile << i << ",";
        pitchFile << pm.rangeStart << "," << pm.mark << "," << pm.rangeEnd << ",";
        pitchFile << pm.getRangeLength() << "\n";
    }

    pitchFile.close();

    // ========== Write Synth Marks CSV ==========
    juce::String synthFilename = timestampedDir + "/synthmarks_" + timestamp + ".csv";
    juce::File synthCsvFile(synthFilename);

    std::ofstream synthFile(synthCsvFile.getFullPathName().toStdString());
    if (!synthFile.is_open())
    {
        DBG("Failed to open file for writing: " + synthFilename);
        return false;
    }

    // Write synth marks CSV header
    synthFile << "Index,SynthRangeStart,SynthMark,SynthRangeEnd,SynthRangeLength,";
    synthFile << "PitchRangeStart,PitchMarkRef,PitchRangeEnd\n";

    // Write synth marks
    for (size_t i = 0; i < synthMarks.size(); ++i)
    {
        const auto& sm = synthMarks[i];
        synthFile << i << ",";
        synthFile << sm.synthRangeStart << "," << sm.synthMark << "," << sm.synthRangeEnd << ",";
        synthFile << sm.getSynthRangeLength() << ",";
        synthFile << sm.pitchRangeStart << "," << sm.pitchMark << "," << sm.pitchRangeEnd << "\n";
    }

    synthFile.close();

    DBG("Successfully wrote marks to: " + timestampedDir);
    DBG("  Pitch marks: " + juce::String(pitchMarks.size()) + " -> " + pitchFilename);
    DBG("  Synth marks: " + juce::String(synthMarks.size()) + " -> " + synthFilename);

    return true;
}
