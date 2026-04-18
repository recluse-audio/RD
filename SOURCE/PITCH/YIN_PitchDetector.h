//
//  Made by Ryan Devens, 2024-07-12
//

namespace YIN_PitchDetectorConstants
{
    static const double DefaultThreshold = 0.15;
    static const int DefaultDetectionSize = 4096;
}

/*
    A class to detect and track pitch using the YIN algorithm.
    Uses cumulative mean normalized difference (CMND) for pitch detection.
*/

#include "Util/Juce_Header.h"
class YIN_PitchDetector
{
	friend class YIN_PitchDetectorTester;
public:

    YIN_PitchDetector();
    ~YIN_PitchDetector();

    void prepareToPlay(int blockSize);

    // split it up however you like, this tells you what pitch is the fundamental in that buffer.
    float process(juce::AudioBuffer<float>& buffer);

    const double getCurrentPitch();
    const double getCurrentPeriod();

    void setThreshold(double threshold) { mThreshold.store(threshold); }
    double getThreshold() { return mThreshold.load(); }

private:
    int mHalfBlock = 0;

    // Allowed amount of uncertainty, inverse of minimum probability needed to count as a pitch
    std::atomic<double> mThreshold = YIN_PitchDetectorConstants::DefaultThreshold;

    std::atomic<double> mCurrentPitchHz = -1.0;
    std::atomic<double> mCurrentPeriod  = -1.0;
    std::atomic<double> mProbability    = -1.0;

    juce::AudioBuffer<float> differenceBuffer;
    juce::AudioBuffer<float> cmndBuffer; // "Cumulative Mean Normalized Difference" buffer
};
