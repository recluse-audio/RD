/**
 * PitchMark.h
 * Created by Ryan Devens
 *
 * Represents a single pitch mark with its associated range.
 */

#pragma once
#include "Util/Juce_Header.h"

/**
 * PitchMark represents a detected pitch mark position along with its valid range.
 *
 * The pitch mark is centered in the range, with:
 * - rangeStart: one detected period before the mark
 * - rangeEnd: one detected period after the mark - 1
 *
 * This range defines the audio segment associated with this pitch period.
 */
class PitchMark
{
public:
    juce::int64 mark;        // The actual pitch mark position (absolute sample count)
    juce::int64 rangeStart;  // Start of valid range: mark - period
    juce::int64 rangeEnd;    // End of valid range: mark + period - 1

    /**
     * Default constructor (creates invalid pitch mark).
     */
    PitchMark()
        : mark(-1)
        , rangeStart(-1)
        , rangeEnd(-1)
    {
    }

    /**
     * Construct a pitch mark with its range based on detected period.
     * @param pitchMark The pitch mark position
     * @param detectedPeriod The detected period in samples
     */
    PitchMark(juce::int64 pitchMark, float detectedPeriod)
        : mark(pitchMark)
        , rangeStart(pitchMark - static_cast<juce::int64>(detectedPeriod))
        , rangeEnd(pitchMark + static_cast<juce::int64>(detectedPeriod) - 1)
    {
    }

    /**
     * Check if this pitch mark is valid.
     * @return True if the mark is valid (>= 0)
     */
    bool isValid() const
    {
        return mark >= 0;
    }

    /**
     * Get the length of the range.
     * @return Range length in samples
     */
    juce::int64 getRangeLength() const
    {
        return rangeEnd - rangeStart + 1;
    }

    /**
     * Get the range as a juce::Range.
     * @return Range object
     */
    juce::Range<juce::int64> getRange() const
    {
        return juce::Range<juce::int64>(rangeStart, rangeEnd + 1);
    }
};
