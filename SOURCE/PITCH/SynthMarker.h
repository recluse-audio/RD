/**
 * SynthMarker.h
 * Created by Ryan Devens
 *
 * Manages synthesis marks for TD-PSOLA synthesis.
 * Creates and owns an array of SynthMark objects based on pitch marks and shifted period.
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PitchMark.h"
#include "SynthMark.h"
#include <vector>

/**
 * SynthMarker generates and manages synthesis marks for TD-PSOLA.
 *
 * Given a batch of pitch marks and a shifted period (output period), it generates
 * synth marks that determine where and how to synthesize output audio.
 *
 * Algorithm:
 * 1. First synth mark matches the first pitch mark (same position)
 * 2. Subsequent synth marks are placed at intervals of shiftedPeriod
 * 3. Each synth mark determines its source pitch mark using the rule:
 *    - If pitch mark range is [rangeStart - mark - rangeEnd]
 *    - All synth marks with centers from [mark to rangeEnd] use that pitch mark
 *    - In other words: use the pitch mark whose [mark, rangeEnd] range contains the synth position
 *
 * Example:
 * - Pitch mark at 128 with range [0, 255]
 * - All synth marks from position 128-255 will read from pitch mark range [0, 255]
 */
class SynthMarker
{
public:
    SynthMarker();
    ~SynthMarker();

    /**
     * Generate synthesis marks from pitch marks.
     *
     * Generates synth marks incrementally by stepping through shiftedPeriod intervals.
     * If a predicted next synth mark exists (from previous call), starts from there.
     * Otherwise, syncs with the first pitch mark position.
     * Continues generating marks until reaching the end of the sample range.
     *
     * @param pitchMarks Array of pitch marks to synthesize from
     * @param shiftedPeriod Output period for placing synth marks (can differ from input period)
     * @param absSampleRange Absolute sample range to generate synth marks for
     */
    void generateSynthMarks(const std::vector<PitchMark>& pitchMarks, float shiftedPeriod, juce::Range<juce::int64> absSampleRange);

    /**
     * Get the generated synth marks.
     * @return Reference to the synth marks array
     */
    const std::vector<SynthMark>& getSynthMarks() const { return mSynthMarks; }

    /**
     * Get the number of stored synth marks.
     * @return Number of synth marks
     */
    int getNumSynthMarks() const { return static_cast<int>(mSynthMarks.size()); }

    /**
     * Get synth marks within a given absolute time range.
     * Returns all synth marks whose center position falls within the query range.
     *
     * @param range Absolute time range to query (in synth/output time)
     * @return Vector of synth marks whose center is in the range
     */
    std::vector<SynthMark> getSynthMarksInRange(juce::Range<juce::int64> range) const;

    /**
     * Reset and clear all synth marks.
     */
    void reset();

private:
    std::vector<SynthMark> mSynthMarks;

    // Predicted position for the next synth mark (for incremental generation)
    // If not set (-1), will sync with first pitch mark on next generation
    juce::int64 mPredictedNextSynthMark = -1;

    /**
     * Find the pitch mark that should be used for a given synth mark position.
     *
     * Uses the rule: synth mark at position P uses the pitch mark where
     * P is in the range [pitchMark.mark, pitchMark.rangeEnd].
     *
     * @param pitchMarks Array of pitch marks to search
     * @param synthPos Synth mark position
     * @return Pointer to the pitch mark to use, or nullptr if not found
     */
    const PitchMark* _findPitchMarkForSynthPos(const std::vector<PitchMark>& pitchMarks, juce::int64 synthPos) const;
};
