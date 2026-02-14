/**
 * PitchMarker.cpp
 * Created by Ryan Devens
 */

#include "PitchMarker.h"
#include <cmath>
#include <algorithm>

//=======================================
PitchMarker::PitchMarker()
{
    // Allocate FIFO for 32 pitch marks
    mMaxPitchMarks = 32;
    mPitchMarks.resize(mMaxPitchMarks, PitchMark()); // Default construct invalid pitch marks
    mPitchMarkWritePos = 0;
    mNumStoredMarks = 0;
}

//=======================================
PitchMarker::~PitchMarker()
{
}

//=======================================
void PitchMarker::reset()
{
    mLastMark = -1;
    mPredictedNextMark = -1;
    mPitchMarkWritePos = 0;
    mNumStoredMarks = 0;
    std::fill(mPitchMarks.begin(), mPitchMarks.end(), PitchMark()); // Reset to invalid pitch marks
}

//=======================================
int PitchMarker::getNumStoredMarks() const
{
    return mNumStoredMarks;
}

//=======================================
juce::int64 PitchMarker::doPitchMarking(const CircularBuffer& circularBuffer, juce::Range<juce::int64> searchRange,
                                        float detectedPeriod, juce::int64 samplesProcessed, bool usePrediction)
{
    // Can't do pitch marking without a valid period
    if (detectedPeriod <= 0.0f)
        return -1;

    // Find the pitch mark
    juce::int64 foundMark = _findMark(circularBuffer, searchRange, detectedPeriod, samplesProcessed, usePrediction);

    // Store the mark in FIFO with its range
    if (foundMark >= 0)
    {
        _storePitchMark(foundMark, detectedPeriod);
    }

    return foundMark;
}

//=======================================
juce::int64 PitchMarker::_findMark(const CircularBuffer& circularBuffer, juce::Range<juce::int64> searchRange,
                                   float detectedPeriod, juce::int64 samplesProcessed, bool usePrediction)
{
    const juce::int64 periodInt = static_cast<juce::int64>(std::llround(detectedPeriod));

    juce::Range<juce::int64> actualSearchRange = searchRange;

    // If we have a prediction and we're asked to use it, narrow the search range
    if (usePrediction && mPredictedNextMark > 0)
    {
        // Search within +/- 25% of the period around the prediction
        const juce::int64 radius = periodInt / 4;
        actualSearchRange = juce::Range<juce::int64>(
            mPredictedNextMark - radius,
            mPredictedNextMark + radius);
    }

    // Find peak in the search range
    juce::int64 foundMark = circularBuffer.findPeakInRange(actualSearchRange, 0);

    // Use correlation refinement if we have enough processing history
    // Need at least 2 periods of history before foundMark for correlation to work
    const juce::int64 minSamplesForCorrelation = periodInt * 2;
    if (samplesProcessed >= minSamplesForCorrelation && foundMark >= minSamplesForCorrelation)
    {
        // Refine using correlation for phase continuity
        foundMark = _refineMarkByCorrelation(circularBuffer, foundMark, detectedPeriod);
    }

    // Update state
    mLastMark = foundMark;
    mPredictedNextMark = foundMark + periodInt;

    return foundMark;
}

//=======================================
void PitchMarker::_storePitchMark(juce::int64 pitchMark, float detectedPeriod)
{
    // Create PitchMark with range: [mark - period, mark + period - 1]
    mPitchMarks[mPitchMarkWritePos] = PitchMark(pitchMark, detectedPeriod);
    mPitchMarkWritePos = (mPitchMarkWritePos + 1) % mMaxPitchMarks;

    // Track number of stored marks (saturates at mMaxPitchMarks)
    if (mNumStoredMarks < mMaxPitchMarks)
    {
        mNumStoredMarks++;
    }
}

//=======================================
PitchMark PitchMarker::getLastPitchMark() const
{
    if (mNumStoredMarks == 0)
        return PitchMark(); // Return invalid pitch mark

    // Get the last written pitch mark (one position before current write position)
    int lastIndex = (mPitchMarkWritePos - 1 + mMaxPitchMarks) % mMaxPitchMarks;
    return mPitchMarks[lastIndex];
}

//=======================================
std::vector<PitchMark> PitchMarker::getPitchMarksInRange(juce::Range<juce::int64> range) const
{
    std::vector<PitchMark> result;
    result.reserve(mNumStoredMarks);

    // Iterate through stored pitch marks
    for (int i = 0; i < mNumStoredMarks; ++i)
    {
        const PitchMark& pm = mPitchMarks[i];

        if (!pm.isValid())
            continue;

        // Check if pitch mark's center position is within the query range
        if (range.contains(pm.mark))
        {
            result.push_back(pm);
        }
    }

    return result;
}

//=======================================
juce::int64 PitchMarker::_refineMarkByCorrelation(const CircularBuffer& circularBuffer, juce::int64 candidateMark, float detectedPeriod) const
{
    const int P = static_cast<int>(std::llround(detectedPeriod));
    const int radius = std::max(1, P / 4);

    // Reference cycle: one period ending at candidateMark
    std::vector<float> ref(P);
    for (int i = 0; i < P; ++i)
    {
        ref[i] = _readMonoSample(circularBuffer, candidateMark - P + i);
    }

    double bestScore = -1.0;
    juce::int64 bestMark = candidateMark;

    // Search for best correlation within the radius
    for (int off = -radius; off <= radius; ++off)
    {
        const juce::int64 cand = candidateMark + off;

        double num = 0.0, denA = 0.0, denB = 0.0;
        for (int i = 0; i < P; ++i)
        {
            const float a = ref[i];
            const float b = _readMonoSample(circularBuffer, cand - P + i);
            num  += static_cast<double>(a) * static_cast<double>(b);
            denA += static_cast<double>(a) * static_cast<double>(a);
            denB += static_cast<double>(b) * static_cast<double>(b);
        }

        // Calculate normalized correlation coefficient
        const double score = num / (std::sqrt(denA * denB) + 1e-12);
        if (score > bestScore)
        {
            bestScore = score;
            bestMark = cand;
        }
    }

    return bestMark;
}

//=======================================
inline float PitchMarker::_readMonoSample(const CircularBuffer& circularBuffer, juce::int64 sampleIndex) const
{
    const int ch = 0;
    return circularBuffer.getSample(ch, sampleIndex);
}
