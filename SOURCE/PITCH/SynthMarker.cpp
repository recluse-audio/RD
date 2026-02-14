/**
 * SynthMarker.cpp
 * Created by Ryan Devens
 */

#include "SynthMarker.h"

//=======================================
SynthMarker::SynthMarker()
{
}

//=======================================
SynthMarker::~SynthMarker()
{
}

//=======================================
void SynthMarker::reset()
{
    mSynthMarks.clear();
}

//=======================================
void SynthMarker::generateSynthMarks(const std::vector<PitchMark>& pitchMarks, float shiftedPeriod, int numSynthMarks)
{
    // Clear existing synth marks
    mSynthMarks.clear();

    // Can't generate without pitch marks
    if (pitchMarks.empty() || numSynthMarks <= 0)
        return;

    // Find first valid pitch mark
    const PitchMark* firstPitchMark = nullptr;
    for (const auto& pm : pitchMarks)
    {
        if (pm.isValid())
        {
            firstPitchMark = &pm;
            break;
        }
    }

    if (!firstPitchMark)
        return; // No valid pitch marks

    // Reserve space for synth marks
    mSynthMarks.reserve(numSynthMarks);

    // Generate synth marks
    for (int i = 0; i < numSynthMarks; ++i)
    {
        // Calculate synth mark position
        // First synth mark matches first pitch mark, subsequent marks use shiftedPeriod
        juce::int64 synthPos = firstPitchMark->mark + static_cast<juce::int64>(i * shiftedPeriod);

        // Find which pitch mark this synth position should use
        const PitchMark* sourcePitchMark = _findPitchMarkForSynthPos(pitchMarks, synthPos);

        if (sourcePitchMark)
        {
            // Create synth mark from the source pitch mark
            SynthMark synthMark(*sourcePitchMark, synthPos, shiftedPeriod);
            mSynthMarks.push_back(synthMark);
        }
        else
        {
            // No valid pitch mark found for this position, create invalid synth mark
            mSynthMarks.push_back(SynthMark());
        }
    }
}

//=======================================
std::vector<SynthMark> SynthMarker::getSynthMarksInRange(juce::Range<juce::int64> range) const
{
    std::vector<SynthMark> result;
    result.reserve(mSynthMarks.size());

    // Iterate through stored synth marks
    for (const auto& sm : mSynthMarks)
    {
        if (!sm.isValid())
            continue;

        // Check if synth mark's center position is within the query range
        if (range.contains(sm.synthMark))
        {
            result.push_back(sm);
        }
    }

    return result;
}

//=======================================
const PitchMark* SynthMarker::_findPitchMarkForSynthPos(const std::vector<PitchMark>& pitchMarks, juce::int64 synthPos) const
{
    // Find the pitch mark whose range [mark, rangeEnd] contains synthPos
    // Rule: synth marks from [pitchMark.mark to pitchMark.rangeEnd] use that pitch mark

    const PitchMark* bestMatch = nullptr;

    for (const auto& pitchMark : pitchMarks)
    {
        if (!pitchMark.isValid())
            continue;

        // Check if synthPos falls in the range [mark, rangeEnd]
        if (synthPos >= pitchMark.mark && synthPos <= pitchMark.rangeEnd)
        {
            // Found a match
            bestMatch = &pitchMark;
            // Keep the first match (earliest pitch mark that contains this position)
            break;
        }
    }

    // If no match found in forward ranges, use the last valid pitch mark
    // This handles synth marks that extend beyond the last pitch mark
    if (!bestMatch)
    {
        for (auto it = pitchMarks.rbegin(); it != pitchMarks.rend(); ++it)
        {
            if (it->isValid())
            {
                bestMatch = &(*it);
                break;
            }
        }
    }

    return bestMatch;
}
