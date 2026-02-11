/**
 * TD_PitchDetector.h
 * Created by Ryan Devens
 *
 * Time-domain pitch detector using FFT-based autocorrelation.
 * Extracted from TD-PSOLA implementation.
 */

#pragma once
#include "Util/Juce_Header.h"
#include <memory>

namespace TD_PitchDetectorConstants
{
    static constexpr float kDefaultMinHz = 80.0f;   // ~E2
    static constexpr float kDefaultMaxHz = 400.0f;  // ~G4
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
 * This is the pitch detection method from TD-PSOLA.
 */
class TD_PitchDetector
{
public:
    TD_PitchDetector();
    ~TD_PitchDetector();

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

    /**
     * Get the last detected period.
     * @return Period in samples, or -1 if no pitch detected
     */
    float getCurrentPeriod() const { return mCurrentPeriod; }

    /**
     * Set the pitch detection frequency range.
     * @param minHz Minimum frequency in Hz (default 80 Hz)
     * @param maxHz Maximum frequency in Hz (default 400 Hz)
     */
    void setFrequencyRange(float minHz, float maxHz);

    /**
     * Get minimum period in samples (based on maxHz).
     */
    int getMinPeriod() const { return mMinPeriod; }

    /**
     * Get maximum period in samples (based on minHz).
     */
    int getMaxPeriod() const { return mMaxPeriod; }

private:
    /**
     * Update min/max period based on current frequency range and sample rate.
     */
    void _updatePeriodBounds();

    // State
    double mSampleRate = 44100.0;
    float mCurrentPeriod = -1.0f;
    float mMinHz = TD_PitchDetectorConstants::kDefaultMinHz;
    float mMaxHz = TD_PitchDetectorConstants::kDefaultMaxHz;
    int mMinPeriod = 0;
    int mMaxPeriod = 0;

    // FFT resources
    std::unique_ptr<juce::dsp::FFT> mFFT;
    juce::AudioBuffer<float> mFFTBuffer;
};
