/**
 * SynthMark.cpp
 * Created by Ryan Devens
 */

#include "SynthMark.h"
#include "PitchMark.h"

//=======================================
SynthMark::SynthMark(const PitchMark& pitchMark, juce::int64 synthMarkPosition, float outputPeriod)
    : pitchMark(pitchMark.mark)
    , pitchRangeStart(pitchMark.rangeStart)
    , pitchRangeEnd(pitchMark.rangeEnd)
    , synthMark(synthMarkPosition)
    , synthRangeStart(synthMarkPosition - static_cast<juce::int64>(outputPeriod))
    , synthRangeEnd(synthMarkPosition + static_cast<juce::int64>(outputPeriod) - 1)
{
}
