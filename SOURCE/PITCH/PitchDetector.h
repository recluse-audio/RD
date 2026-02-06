//
//  Made by Ryan Devens, 2024-07-12
//

namespace PitchDetectorMagicNumbers
{
    static const double DefaultThreshold = 0.15;
    static const int DefaultDetectionSize = 4096; 
}

/*
    A class to detect and track pitch.
    ATP handles this with two classes I believe, but I will handle it will a single class and two modes.
    Detection and Tracking.

    There are different optimizations and callbacks once we have "detected" a pitch and enter tracking mode
*/

#include "Util/Juce_Header.h"
class PitchDetector
{
	friend class PitchDetectorTester;
public:


    PitchDetector();
    ~PitchDetector();

    void prepareToPlay(int blockSize);

    // split it up however you like, this tells you what pitch is the fundamental in that buffer.
    float process(juce::AudioBuffer<float>& buffer);

    const double getCurrentPitch();
    const double getCurrentPeriod();

    void setThreshold(double threshold) { mThreshold.store(threshold); }
    double getThreshold() { return mThreshold.load(); }

private:
    // Conditions of the environment
    int mHalfBlock = 0;

    // Allowed amount of uncertainty, inverse of minimum probability needed to count as a pitch
    std::atomic<double> mThreshold = PitchDetectorMagicNumbers::DefaultThreshold;

    // These are what we are calculating for
    std::atomic<double> mCurrentPitchHz = -1.0;
    std::atomic<double> mCurrentPeriod = -1.0;
    std::atomic<double> mProbability = -1.0;

    juce::AudioBuffer<float> differenceBuffer;
    juce::AudioBuffer<float> cmndBuffer; // "Cumulative Mean Normalized Difference" buffer, you can thank YIN for this abbrev.



};
