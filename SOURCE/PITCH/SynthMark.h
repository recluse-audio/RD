/**
 * SynthMark.h
 * Created by Ryan Devens
 *
 * Represents a synthesis mark that references pitch mark data from the circular buffer.
 * Stores a copy of pitch mark data (not a reference) to avoid issues with FIFO resets.
 */

#pragma once
#include "Util/Juce_Header.h"

/**
 * SynthMark represents a synthesis position that references a pitch mark's audio data.
 *
 * Each SynthMark contains:
 * - A copy of the pitch mark data (position and range in circular buffer)
 * - Its own synth mark position data (where it will be synthesized in output)
 *
 * The pitch mark data is copied (not referenced) so that it remains valid even if
 * the PitchMarker's FIFO is reset or the original PitchMark is overwritten.
 * 
 * **IMPORTANT** - THIS IS IN THE SAME TIME CONTEXT AS PITCH MARK
 * So if the detection range is delayed, the synth marks will be too 
 */
class SynthMark
{
public:
    // Pitch mark data (copied from PitchMark to avoid reference issues)
    juce::int64 pitchMark;        // The pitch mark position in circular buffer (absolute sample count)
    juce::int64 pitchRangeStart;  // Start of pitch mark's valid range: mark - period
    juce::int64 pitchRangeEnd;    // End of pitch mark's valid range: mark + period - 1

    // Synth mark data (output position)
    juce::int64 synthMark;        // The synth mark position in output (absolute sample count)
    juce::int64 synthRangeStart;  // Start of synth mark's output range
    juce::int64 synthRangeEnd;    // End of synth mark's output range

    // Windowing flag: false for inharmonic/unvoiced content — grain uses rectangular window
    bool isVoiced = true;

    /**
     * Default constructor (creates invalid synth mark).
     */
    SynthMark()
        : pitchMark(-1)
        , pitchRangeStart(-1)
        , pitchRangeEnd(-1)
        , synthMark(-1)
        , synthRangeStart(-1)
        , synthRangeEnd(-1)
    {
    }

    /**
     * Construct a synth mark from pitch mark data and synth position.
     * @param pitchMarkPosition The pitch mark position in circular buffer
     * @param pitchMarkRangeStart Start of pitch mark range
     * @param pitchMarkRangeEnd End of pitch mark range
     * @param synthMarkPosition The synth mark position in output
     * @param outputPeriod The output period for synth range (can differ from input period)
     */
    SynthMark(juce::int64 pitchMarkPosition, juce::int64 pitchMarkRangeStart, juce::int64 pitchMarkRangeEnd,
              juce::int64 synthMarkPosition, float outputPeriod)
        : pitchMark(pitchMarkPosition)
        , pitchRangeStart(pitchMarkRangeStart)
        , pitchRangeEnd(pitchMarkRangeEnd)
        , synthMark(synthMarkPosition)
        , synthRangeStart(synthMarkPosition - static_cast<juce::int64>(outputPeriod))
        , synthRangeEnd(synthMarkPosition + static_cast<juce::int64>(outputPeriod) - 1)
    {
    }

    /**
     * Construct a synth mark from a PitchMark and synth position.
     * @param pitchMark The pitch mark to copy data from
     * @param synthMarkPosition The synth mark position in output
     * @param outputPeriod The output period for synth range
     */
    SynthMark(const class PitchMark& pitchMark, juce::int64 synthMarkPosition, float outputPeriod);

    /**
     * Check if this synth mark is valid.
     * @return True if both pitch mark and synth mark are valid (>= 0)
     */
    bool isValid() const
    {
        return pitchMark >= 0 && synthMark >= 0;
    }

    /**
     * Get the length of the pitch range.
     * @return Pitch range length in samples
     */
    juce::int64 getPitchRangeLength() const
    {
        return pitchRangeEnd - pitchRangeStart + 1;
    }

    /**
     * Get the length of the synth range.
     * @return Synth range length in samples
     */
    juce::int64 getSynthRangeLength() const
    {
        return synthRangeEnd - synthRangeStart + 1;
    }

    /**
     * Get the pitch range as a juce::Range.
     * @return Pitch range object
     */
    juce::Range<juce::int64> getPitchRange() const
    {
        return juce::Range<juce::int64>(pitchRangeStart, pitchRangeEnd + 1);
    }

    /**
     * Get the synth range as a juce::Range.
     * @return Synth range object
     */
    juce::Range<juce::int64> getSynthRange() const
    {
        return juce::Range<juce::int64>(synthRangeStart, synthRangeEnd + 1);
    }
};
