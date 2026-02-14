/**
 * PitchMarker.h
 * Created by Ryan Devens
 *
 * Finds and tracks pitch marks (peaks) in audio for TD-PSOLA.
 * Operates in absolute sample count coordinates.
 */

#pragma once
#include "Util/Juce_Header.h"
#include "../CircularBuffer.h"
#include "PitchMark.h"

/**
 * Finds and tracks pitch marks for TD-PSOLA pitch shifting.
 *
 * A pitch mark is a specific sample position (typically a peak) that represents
 * the start of a pitch period. PitchMarker finds these marks and tracks them
 * across time using prediction and correlation refinement.
 *
 * Key features:
 * - Peak-based mark detection within search ranges
 * - Correlation refinement for phase continuity
 * - Prediction tracking (predicts next mark based on detected period)
 * - Works in absolute sample count (not wrapped buffer indices)
 * - Owns a FIFO buffer of pitch marks for historical tracking
 */
class PitchMarker
{
public:
    PitchMarker();
    ~PitchMarker();

    /**
     * Perform pitch marking on the given range.
     *
     * This is the main method that finds a pitch mark and stores it in the internal FIFO.
     * It uses the circular buffer, search range, and detected period to find a peak,
     * refine it with correlation, and store it in the pitch mark FIFO.
     *
     * @param circularBuffer Audio buffer to search
     * @param searchRange Search range in absolute sample count
     * @param detectedPeriod Detected period in samples (from pitch detector)
     * @param samplesProcessed Total samples processed so far (for correlation history check)
     * @param usePrediction If true, uses internal prediction to narrow search range
     * @return Detected pitch mark in absolute sample count, or -1 if invalid period
     */
    juce::int64 doPitchMarking(const CircularBuffer& circularBuffer, juce::Range<juce::int64> searchRange,
                               float detectedPeriod, juce::int64 samplesProcessed, bool usePrediction = true);

    /**
     * Get the last found pitch mark.
     * @return Last pitch mark in absolute sample count, or -1 if none found yet
     */
    juce::int64 getLastMark() const { return mLastMark; }

    /**
     * Get the predicted next pitch mark.
     * @return Predicted pitch mark in absolute sample count, or -1 if no prediction available
     */
    juce::int64 getPredictedNextMark() const { return mPredictedNextMark; }

    /**
     * Get the stored pitch marks FIFO.
     * @return Reference to the pitch marks vector (circular FIFO)
     */
    const std::vector<PitchMark>& getPitchMarks() const { return mPitchMarks; }

    /**
     * Get the number of stored pitch marks.
     * @return Number of valid pitch marks in the FIFO (may be less than FIFO size)
     */
    int getNumStoredMarks() const;

    /**
     * Get the current write position in the FIFO.
     * @return Write position index
     */
    int getPitchMarkWritePos() const { return mPitchMarkWritePos; }

    /**
     * Get the last stored PitchMark object.
     * @return Last pitch mark, or invalid PitchMark if none stored
     */
    PitchMark getLastPitchMark() const;

    /**
     * Get pitch marks within a given absolute time range.
     * Returns all pitch marks whose center position falls within the query range.
     *
     * @param range Absolute time range to query
     * @return Vector of pitch marks whose center is in the range
     */
    std::vector<PitchMark> getPitchMarksInRange(juce::Range<juce::int64> range) const;

    /**
     * Reset the marker state (clears history, prediction, and pitch marks).
     */
    void reset();

private:
    /**
     * Find a pitch mark in the circular buffer.
     *
     * This method finds a peak (pitch mark) within the given search range,
     * optionally using prediction from the previous mark to narrow the search.
     * If enough audio history exists, it refines the mark using correlation
     * for phase continuity.
     *
     * @param circularBuffer Audio buffer to search
     * @param searchRange Initial search range in absolute sample count
     * @param detectedPeriod Detected period in samples (from pitch detector)
     * @param samplesProcessed Total samples processed so far (for correlation history check)
     * @param usePrediction If true, uses internal prediction to narrow search range
     * @return Detected pitch mark in absolute sample count
     */
    juce::int64 _findMark(const CircularBuffer& circularBuffer, juce::Range<juce::int64> searchRange,
                          float detectedPeriod, juce::int64 samplesProcessed, bool usePrediction);

    /**
     * Refine a pitch mark using correlation with the previous period.
     *
     * Compares the waveform cycle at the candidate mark with the previous cycle
     * to find the best phase-aligned position within a small radius.
     *
     * @param circularBuffer Audio buffer to read from
     * @param candidateMark Initial pitch mark estimate
     * @param detectedPeriod Period in samples
     * @return Refined pitch mark in absolute sample count
     */
    juce::int64 _refineMarkByCorrelation(const CircularBuffer& circularBuffer, juce::int64 candidateMark,
                                        float detectedPeriod) const;

    /**
     * Read a single mono sample from the circular buffer.
     * @param circularBuffer Buffer to read from
     * @param sampleIndex Sample index in absolute sample count (will be wrapped)
     * @return Sample value
     */
    inline float _readMonoSample( const CircularBuffer& circularBuffer, juce::int64 sampleIndex) const;

    /**
     * Store a pitch mark in the FIFO.
     * @param pitchMark The pitch mark position
     * @param detectedPeriod The detected period in samples
     */
    void _storePitchMark(juce::int64 pitchMark, float detectedPeriod);

    // State
    juce::int64 mLastMark = -1;           // Last detected pitch mark (absolute sample count)
    juce::int64 mPredictedNextMark = -1;  // Predicted next pitch mark (absolute sample count)

    // Pitch mark storage (circular FIFO)
    std::vector<PitchMark> mPitchMarks;
    int mPitchMarkWritePos = 0;
    int mMaxPitchMarks = 0;
    int mNumStoredMarks = 0;  // Tracks actual number of marks stored (up to mMaxPitchMarks)
};
