/**
 * SynthMark.cpp
 * Created by Ryan Devens
 */

#include "SynthMark.h"
#include "PitchMark.h"

//=======================================
SynthMark::SynthMark(const PitchMark& pitchMark, juce::int64 synthMarkPosition)
    : pitchMark(pitchMark.mark)
    , pitchRangeStart(pitchMark.rangeStart)
    , pitchRangeEnd(pitchMark.rangeEnd)
    , synthMark(synthMarkPosition)
{
    // Synth range must match pitch range length
    // Calculate pitch period from the pitch mark position and range start
    juce::int64 pitchPeriod = pitchMark.mark - pitchMark.rangeStart;
    synthRangeStart = synthMarkPosition - pitchPeriod;
    synthRangeEnd = synthMarkPosition + pitchPeriod - 1;
}
