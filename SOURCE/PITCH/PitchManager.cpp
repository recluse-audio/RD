/**
 * PitchManager.cpp
 * Created by Ryan Devens
 */

#include "PitchManager.h"
#include <algorithm>

//=======================================
PitchManager::PitchManager()
{
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

    // Prepare pitch detector
    mPitchDetector.prepare(sampleRate);

    // Prepare pitch marker
    mPitchMarker.prepare(sampleRate, detectionWindowSize, PitchManagerConstants::kPitchMarkBufferSeconds);

    // Allocate detection buffer (mono, to be filled incrementally)
    mDetectionBuffer.setSize(1, mDetectionWindowSize);
    mDetectionBuffer.clear();
    mDetectionBufferFillPos = 0;

    reset();
}

//=======================================
void PitchManager::reset()
{
    mAbsoluteSampleCounter = 0;
    mCurrentPeriod = -1.0f;
    mDetectionBufferFillPos = 0;
    mDetectionBuffer.clear();
    mPitchMarker.reset();
}

//=======================================
bool PitchManager::process(const juce::AudioBuffer<float>& buffer, CircularBuffer& circularBuffer)
{
    const int numSamples = buffer.getNumSamples();
    bool detectionOccurred = false;

    // Write incoming audio to circular buffer
    circularBuffer.pushBuffer(const_cast<juce::AudioBuffer<float>&>(buffer));

    // Accumulate audio in detection buffer
    int samplesProcessed = 0;
    while (samplesProcessed < numSamples)
    {
        const int samplesToProcess = std::min(numSamples - samplesProcessed, mDetectionWindowSize - mDetectionBufferFillPos);

        // Copy samples to detection buffer
        for (int i = 0; i < samplesToProcess; ++i)
        {
            const float sample = buffer.getSample(0, samplesProcessed + i);
            mDetectionBuffer.setSample(0, mDetectionBufferFillPos + i, sample);
        }

        mDetectionBufferFillPos += samplesToProcess;
        samplesProcessed += samplesToProcess;
        mAbsoluteSampleCounter += samplesToProcess;

        // Run detection when buffer is full
        if (mDetectionBufferFillPos >= mDetectionWindowSize)
        {
            mCurrentPeriod = _runDetection();
            mDetectionBufferFillPos = 0;
            detectionOccurred = true;
        }
    }

    return detectionOccurred;
}

//=======================================
juce::int64 PitchManager::findPitchMark(const CircularBuffer& circularBuffer, juce::Range<juce::int64> searchRange, bool usePrediction)
{
    // Can't find pitch marks without a valid period
    if (mCurrentPeriod <= 0.0f)
        return -1;

    // Use PitchMarker to perform pitch marking (finds and stores the mark)
    juce::int64 foundMark = mPitchMarker.doPitchMarking(circularBuffer, searchRange, mCurrentPeriod, mAbsoluteSampleCounter, usePrediction);

    return foundMark;
}

//=======================================
float PitchManager::_runDetection()
{
    // Run pitch detection on the accumulated buffer
    float detectedPeriod = mPitchDetector.process(mDetectionBuffer);
    return detectedPeriod;
}
