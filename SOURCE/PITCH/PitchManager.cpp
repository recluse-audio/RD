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
    mDetectionWindowSize.set(detectionWindowSize);

    mPitchDetector.prepare(sampleRate);

    // Allocate to max so subsequent setDetectionWindowSize calls never reallocate.
    mDetectionBuffer.setSize(numChannels, PitchManagerConstants::kMaxDetectionWindowSize);
    mDetectionBuffer.clear();
    mDetectionBuffer.setSize(numChannels, detectionWindowSize, false, false, true);

    reset();
}

void PitchManager::setDetectionWindowSize(int newSize)
{
    newSize = juce::jlimit(64, PitchManagerConstants::kMaxDetectionWindowSize, newSize);
    mDetectionWindowSize.set(newSize);

    const int numChannels = mDetectionBuffer.getNumChannels();
    if (numChannels > 0)
        mDetectionBuffer.setSize(numChannels, newSize, false, false, true);
}

void PitchManager::setHopSize(int newSize)
{
    newSize = juce::jmax(1, newSize);
    mHopSize.set(newSize);
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
    const int windowSize = mDetectionWindowSize.get();
    const juce::int64 windowEnd = startAbsIndex + windowSize;
    const juce::Range<juce::int64> windowRange(startAbsIndex, windowEnd);
    const int maxIterations = static_cast<int>(windowSize / mCurrentPeriod) + 4;

    for (int i = 0; i < maxIterations; ++i)
    {
        const juce::int64 predicted = mPitchMarker->getPredictedNextMark();
        if (predicted > 0 && predicted >= windowEnd)
            break;

        const juce::int64 mark = mPitchMarker->doPitchMarking(circularBuffer, windowRange, mCurrentPeriod, windowEnd, true);

        if (mark < 0 || mark >= windowEnd)
            break;
    }

    // Generate synth marks using the shifted output period
    const float safeShift     = std::max(shiftRatio, 0.01f);
    const float shiftedPeriod = mCurrentPeriod / safeShift;
    mSynthMarker->generateSynthMarks(mPitchMarker->getPitchMarks(), shiftedPeriod, windowRange);

    return mCurrentPeriod;
}
