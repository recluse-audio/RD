/**
 * TD_PitchDetector.cpp
 * Created by Ryan Devens
 *
 * FFT-based autocorrelation pitch detection extracted from TD-PSOLA.
 */

#include "TD_PitchDetector.h"
#include <cmath>
#include <algorithm>

//=======================================
TD_PitchDetector::TD_PitchDetector()
{
}

//=======================================
TD_PitchDetector::~TD_PitchDetector()
{
}

//=======================================
void TD_PitchDetector::prepare(double sampleRate)
{
    mSampleRate = sampleRate;
    _updatePeriodBounds();
}

//=======================================
void TD_PitchDetector::setFrequencyRange(float minHz, float maxHz)
{
    mMinHz = minHz;
    mMaxHz = maxHz;
    _updatePeriodBounds();
}

//=======================================
void TD_PitchDetector::_updatePeriodBounds()
{
    mMinPeriod = static_cast<int>(mSampleRate / mMaxHz);
    mMaxPeriod = static_cast<int>(mSampleRate / mMinHz);
}

//=======================================
float TD_PitchDetector::process(const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float* signalData = buffer.getReadPointer(0);

    // Ensure FFT size accommodates buffer length
    int fftOrder = static_cast<int>(std::ceil(std::log2(numSamples)));
    int fftSize = 1 << fftOrder;

    // Recreate FFT if size changed
    if (!mFFT || mFFT->getSize() != fftSize)
    {
        mFFT = std::make_unique<juce::dsp::FFT>(fftOrder);
    }

    // Allocate FFT buffer (needs 2x space: first half = input, full buffer = complex output)
    mFFTBuffer.setSize(1, fftSize * 2, false, true, false);
    mFFTBuffer.clear();
    float* fftData = mFFTBuffer.getWritePointer(0);

    // Copy signal data to first half of buffer (per JUCE documentation)
    for (int i = 0; i < numSamples; i++)
    {
        fftData[i] = signalData[i];
    }

    // Forward FFT - use real-only for real input signal
    mFFT->performRealOnlyForwardTransform(fftData, false);

    // Remove DC component
    fftData[0] = 0.0f;

    // Compute power spectrum: fourier * conj(fourier) = |fourier|^2
    // After real-only forward transform, data is in frequency domain
    for (int i = 0; i < fftSize; i++)
    {
        float real = fftData[i * 2];
        float imag = fftData[i * 2 + 1];
        float powerSpec = real * real + imag * imag;
        fftData[i * 2] = powerSpec;
        fftData[i * 2 + 1] = 0.0f;
    }

    // Inverse FFT to get autocorrelation
    mFFT->performRealOnlyInverseTransform(fftData);

    // After inverse transform, autocorrelation is in first half of buffer
    // Find peak in autocorrelation within period range
    int peakIndex = mMinPeriod;
    float peakValue = fftData[mMinPeriod];

    for (int i = mMinPeriod + 1; i < std::min(mMaxPeriod, fftSize / 2); i++)
    {
        float value = fftData[i];
        if (value > peakValue)
        {
            peakValue = value;
            peakIndex = i;
        }
    }

    // Store and return detected period
    mCurrentPeriod = static_cast<float>(peakIndex);
    return mCurrentPeriod;
}
