/**
 * MarkWriter.h
 * Created by Ryan Devens
 *
 * Utility for writing pitch marks and synth marks to CSV files for analysis.
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PITCH/PitchMark.h"
#include "PITCH/SynthMark.h"
#include <vector>
#include <string>

/**
 * MarkWriter provides utilities for writing pitch marks and synth marks to CSV files.
 *
 * Creates timestamped output directories and writes mark data in CSV format
 * for analysis and debugging.
 */
class MarkWriter
{
public:
    /**
     * Write pitch marks and synth marks to a CSV file.
     *
     * Creates a timestamped directory structure:
     * outputDir/MARKS/MARKS_<datetime>/marks_<datetime>.csv
     *
     * CSV format includes both pitch marks and synth marks in one file with columns:
     * Type, MarkPosition, RangeStart, RangeEnd, [SynthMark-specific columns...]
     *
     * @param pitchMarks Vector of pitch marks to write
     * @param synthMarks Vector of synth marks to write
     * @param outputDir Base output directory (e.g., "TESTS/OUTPUT")
     * @return True if write was successful, false otherwise
     */
    static bool writeMarksToCSV(const std::vector<PitchMark>& pitchMarks,
                                const std::vector<SynthMark>& synthMarks,
                                const juce::String& outputDir);

    /**
     * Get current timestamp as a formatted string.
     * Format: YYYYMMDD_HHMMSS
     * @return Timestamp string
     */
    static juce::String getCurrentTimestamp();

private:
    /**
     * Create directory structure if it doesn't exist.
     * @param path Directory path to create
     * @return True if directory exists or was created successfully
     */
    static bool createDirectoryIfNeeded(const juce::String& path);
};
