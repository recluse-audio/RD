#include "YIN_PitchDetector.h"
#include "../BufferMath.h" // YIN stuff is here

//
YIN_PitchDetector::YIN_PitchDetector()
{
	differenceBuffer.setSize(1, YIN_PitchDetectorConstants::DefaultDetectionSize / 2);
	cmndBuffer.setSize(1, YIN_PitchDetectorConstants::DefaultDetectionSize / 2);
}

//
YIN_PitchDetector::~YIN_PitchDetector()
{

}

//
void YIN_PitchDetector::prepareToPlay(int detectionSize)
{
	mHalfBlock = (detectionSize / 2);

	differenceBuffer.setSize(1, mHalfBlock);
	differenceBuffer.clear();
	cmndBuffer.setSize(1, mHalfBlock);
	cmndBuffer.clear();
}


//
float YIN_PitchDetector::process(juce::AudioBuffer<float>& buffer)
{
	float periodEstimate = -1.f;
	differenceBuffer.clear();
	cmndBuffer.clear();
	// /* Step 1: Calculates the squared difference of the signal with a shifted version of itself. */
	BufferMath::yin_difference(buffer, differenceBuffer, mHalfBlock-1);

	BufferMath::yin_normalized_difference(differenceBuffer, cmndBuffer);

	int tauEstimate = BufferMath::yin_absolute_threshold(cmndBuffer, mThreshold);

	if(tauEstimate > 0)
	{
		// Parabolic interpolation to refine period estimate
		periodEstimate = BufferMath::yin_parabolic_interpolation(cmndBuffer, tauEstimate);
		mCurrentPeriod.store(periodEstimate);
	}

	return periodEstimate;
}

//
const double YIN_PitchDetector::getCurrentPitch()
{
	return mCurrentPitchHz.load();
}

const double YIN_PitchDetector::getCurrentPeriod()
{
	return mCurrentPeriod.load();
}

//
