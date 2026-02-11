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
}

//=======================================
juce::int64 PitchMarker::findMark(
    const CircularBuffer& circularBuffer,
    juce::Range<juce::int64> searchRange,
    float detectedPeriod,
    juce::int64 samplesProcessed,
    bool usePrediction)
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
juce::int64 PitchMarker::_refineMarkByCorrelation(
    const CircularBuffer& circularBuffer,
    juce::int64 candidateMark,
    float detectedPeriod) const
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
