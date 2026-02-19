/**
 * PitchManager.cpp
 * Created by Ryan Devens
 */

#include "PitchManager.h"

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
void PitchManager::prepare(double sampleRate, int detectionWindowSize)
{
    mSampleRate = sampleRate;
    mDetectionWindowSize = detectionWindowSize;

    mPitchDetector.prepare(sampleRate);

    mDetectionBuffer.setSize(1, mDetectionWindowSize);
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
float PitchManager::detect(CircularBuffer& circularBuffer, juce::int64 startAbsIndex)
{
    const int wrappedStart = circularBuffer.getWrappedIndex(startAbsIndex);
    circularBuffer.readRange(mDetectionBuffer, wrappedStart);

    mCurrentPeriod = mPitchDetector.process(mDetectionBuffer);
    return mCurrentPeriod;
}
