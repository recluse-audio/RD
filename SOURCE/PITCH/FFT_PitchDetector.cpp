/**
 * FFT_PitchDetector.cpp
 * Created by Ryan Devens
 *
 * FFT-based autocorrelation pitch detection extracted from TD-PSOLA.
 */

#include "FFT_PitchDetector.h"
#include <cmath>
#include <algorithm>

//=======================================
FFT_PitchDetector::FFT_PitchDetector()
{
}

//=======================================
FFT_PitchDetector::~FFT_PitchDetector()
{
}

//=======================================
void FFT_PitchDetector::prepare(double sampleRate)
{
    mSampleRate = sampleRate;
    _updatePeriodBounds();
}

//=======================================
void FFT_PitchDetector::setFrequencyRange(float minHz, float maxHz)
{
    mMinHz = minHz;
    mMaxHz = maxHz;
    _updatePeriodBounds();
}

//=======================================
void FFT_PitchDetector::_updatePeriodBounds()
{
    mMinPeriod = static_cast<int>(mSampleRate / mMaxHz);
    mMaxPeriod = static_cast<int>(mSampleRate / mMinHz);
}

//=======================================
float FFT_PitchDetector::process(const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float* signalData = buffer.getReadPointer(0);

    // If the analysis window cannot fit two full periods at mMinHz, autocorrelation
    // inside [mMinPeriod, mMaxPeriod] is unreliable — report "no pitch" rather than
    // letting an out-of-range spurious peak drive synthesis.
    if (numSamples < 2 * mMaxPeriod)
    {
        mCurrentPeriod   = -1.0f;
        mLastValidPeriod = -1.0f;
        return mCurrentPeriod;
    }

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
    for (int i = 0; i < fftSize; i++)
    {
        float real = fftData[i * 2];
        float imag = fftData[i * 2 + 1];
        float powerSpec = real * real + imag * imag;
        fftData[i * 2]     = powerSpec;
        fftData[i * 2 + 1] = 0.0f;
    }

    // Inverse FFT to get autocorrelation
    mFFT->performRealOnlyInverseTransform(fftData);

    // Lag-0 autocorrelation = signal energy (post DC removal). Used to normalize
    // the peak value into a confidence score in [0, 1] range.
    const float referenceEnergy = fftData[0];
    const float kEnergyEpsilon  = 1.0e-12f;

    if (referenceEnergy < kEnergyEpsilon)
    {
        mCurrentPeriod    = -1.0f;
        mLastValidPeriod  = -1.0f;
        return mCurrentPeriod;
    }

    const int searchEnd = std::min(mMaxPeriod, fftSize / 2);

    // Find raw peak in autocorrelation within period range
    int   peakIndex = mMinPeriod;
    float peakValue = fftData[mMinPeriod];

    for (int i = mMinPeriod + 1; i < searchEnd; i++)
    {
        const float value = fftData[i];
        if (value > peakValue)
        {
            peakValue = value;
            peakIndex = i;
        }
    }

    // Confidence gate: reject when the autocorr peak is small relative to signal
    // energy — this is the unvoiced / silence rejection.
    const float normalizedPeak = peakValue / referenceEnergy;
    if (normalizedPeak < mThreshold)
    {
        mCurrentPeriod   = -1.0f;
        mLastValidPeriod = -1.0f;
        return mCurrentPeriod;
    }

    // Octave-error correction: when previous detection is valid, score the raw
    // peak against octave-related candidates (prev, 2*prev, prev/2) with a
    // continuity weight. Protects steady pitch from harmonic flips while still
    // allowing real pitch changes when the raw peak strongly dominates.
    int chosenIndex = peakIndex;

    if (mLastValidPeriod > 0.0f)
    {
        const int prev = static_cast<int>(mLastValidPeriod + 0.5f);

        auto inRange = [&] (int lag)
        {
            return lag >= mMinPeriod && lag < searchEnd;
        };

        struct Candidate { int lag; float weight; };
        const Candidate candidates[] = {
            { prev,         1.00f },
            { 2 * prev,     0.85f },
            { prev / 2,     0.85f }
        };

        int   bestLag   = peakIndex;
        float bestScore = peakValue * 0.70f; // raw peak weight when outside family

        for (const auto& c : candidates)
        {
            if (! inRange(c.lag))
                continue;
            const float score = fftData[c.lag] * c.weight;
            if (score > bestScore)
            {
                bestScore = score;
                bestLag   = c.lag;
            }
        }

        // If the raw peak is more than one octave away from prev AND does not
        // dominate continuity candidates by >2x, defer to the continuity pick.
        const float ratio = static_cast<float>(peakIndex) / static_cast<float>(prev);
        const bool  octaveJump = std::fabs(std::log2(ratio)) > 1.0f;

        if (octaveJump)
        {
            float bestContinuity = 0.0f;
            for (const auto& c : candidates)
            {
                if (! inRange(c.lag))
                    continue;
                bestContinuity = std::max(bestContinuity, fftData[c.lag]);
            }
            if (peakValue < 2.0f * bestContinuity)
                chosenIndex = bestLag;
            else
                chosenIndex = peakIndex;
        }
        else
        {
            chosenIndex = bestLag;
        }
    }

    mCurrentPeriod   = static_cast<float>(chosenIndex);
    mLastValidPeriod = mCurrentPeriod;
    return mCurrentPeriod;
}
