/**
 * FFT_PitchDetector.h
 * Created by Ryan Devens
 *
 * Time-domain pitch detector using FFT-based autocorrelation.
 * Extracted from TD-PSOLA implementation.
 */

#pragma once
#include "Util/Juce_Header.h"
#include <memory>

namespace FFT_PitchDetectorConstants
{
    static constexpr float kDefaultMinHz       = 80.0f;    // ~E2
    static constexpr float kDefaultMaxHz       = 1000.0f;  // ~B5 (covers full vocal and most instrumental range)
    static constexpr float kDefaultThreshold   = 0.15f;    // normalized autocorrelation minimum
}

/**
 * Time-domain pitch detector using FFT-based autocorrelation.
 *
 * Algorithm:
 * 1. FFT of input signal
 * 2. Compute power spectrum (magnitude squared)
 * 3. Inverse FFT to get autocorrelation
 * 4. Find peak in autocorrelation within valid period range
 *
 * This is the pitch detection method used in the TD-PSOLA grain shifter.
 */
class FFT_PitchDetector
{
public:
    FFT_PitchDetector();
    ~FFT_PitchDetector();

    /**
     * Prepare the pitch detector.
     * @param sampleRate Current sample rate
     */
    void prepare(double sampleRate);

    /**
     * Process an audio buffer and detect the fundamental period.
     *
     * @param buffer Audio buffer to analyze (mono or stereo - only channel 0 is analyzed)
     * @return Detected period in samples, or -1 if no pitch detected
     */
    float process(const juce::AudioBuffer<float>& buffer);

    float getCurrentPeriod() const { return mCurrentPeriod; }

    /**
     * Set the pitch detection frequency range.
     * @param minHz Minimum frequency in Hz (default 80 Hz)
     * @param maxHz Maximum frequency in Hz (default 1000 Hz)
     */
    void setFrequencyRange(float minHz, float maxHz);

    void  setThreshold (float threshold) { mThreshold = threshold; }
    float getThreshold()           const { return mThreshold; }

    int getMinPeriod() const { return mMinPeriod; }
    int getMaxPeriod() const { return mMaxPeriod; }

private:
    void _updatePeriodBounds();

    double mSampleRate    = 44100.0;
    float  mCurrentPeriod = -1.0f;
    float  mMinHz         = FFT_PitchDetectorConstants::kDefaultMinHz;
    float  mMaxHz         = FFT_PitchDetectorConstants::kDefaultMaxHz;
    float  mThreshold     = FFT_PitchDetectorConstants::kDefaultThreshold;
    int    mMinPeriod     = 0;
    int    mMaxPeriod     = 0;
    float  mLastValidPeriod = -1.0f;

    std::unique_ptr<juce::dsp::FFT> mFFT;
    juce::AudioBuffer<float>        mFFTBuffer;
};
