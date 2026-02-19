/**
 * PitchManager.cpp
 * Created by Ryan Devens
 */

#include "PitchManager.h"
#include <algorithm>

//=======================================
PitchManager::PitchManager()
{
    mPitchMarker = std::make_unique<PitchMarker>();
    mSynthMarker = std::make_unique<SynthMarker>();
}

//=======================================
PitchManager::~PitchManager()
{

}

//=======================================
void PitchManager::prepare(double sampleRate, int numChannels, int detectionWindowSize)
{
    mSampleRate = sampleRate;
    mDetectionWindowSize = detectionWindowSize;

    mPitchDetector.prepare(sampleRate);

    mDetectionBuffer.setSize(numChannels, mDetectionWindowSize);
    mDetectionBuffer.clear();

    reset();
}

//=======================================
void PitchManager::reset()
{
    mCurrentPeriod = -1.0f;
    mDetectionBuffer.clear();
    mPitchMarker->reset();
    mSynthMarker->reset();
}

//=======================================
float PitchManager::detect(CircularBuffer& circularBuffer, juce::int64 startAbsIndex, float shiftRatio)
{
    const int wrappedStart = circularBuffer.getWrappedIndex(startAbsIndex);
    circularBuffer.readRange(mDetectionBuffer, wrappedStart);

    mCurrentPeriod = mPitchDetector.process(mDetectionBuffer);

    // if (mCurrentPeriod <= 0.0f)
    //     return mCurrentPeriod;

    // Generate pitch marks across this detection window
    const juce::int64 windowEnd = startAbsIndex + mDetectionWindowSize;
    const juce::Range<juce::int64> windowRange(startAbsIndex, windowEnd);
    const int maxIterations = static_cast<int>(mDetectionWindowSize / mCurrentPeriod) + 4;

    for (int i = 0; i < maxIterations; ++i)
    {
        const juce::int64 predicted = mPitchMarker->getPredictedNextMark();
        if (predicted > 0 && predicted >= windowEnd)
            break;

        const juce::int64 mark = mPitchMarker->doPitchMarking(
            circularBuffer, windowRange, mCurrentPeriod, windowEnd, true);

        if (mark < 0 || mark >= windowEnd)
            break;
    }

    // Generate synth marks using the shifted output period
    const float safeShift     = std::max(shiftRatio, 0.01f);
    const float shiftedPeriod = mCurrentPeriod / safeShift;
    const int   numSynthMarks = static_cast<int>(mDetectionWindowSize / shiftedPeriod) + 4;
    mSynthMarker->generateSynthMarks(mPitchMarker->getPitchMarks(), shiftedPeriod, numSynthMarks);

    return mCurrentPeriod;
}
