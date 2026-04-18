/**
 * Granulator.cpp
 * Created by Ryan Devens
 *
 * Implementation of Granulator.
 */

#include "Granulator.h"

Granulator::Granulator(CircularBuffer& sourceBuffer)
    : mSourceBuffer(sourceBuffer)
{
}

Granulator::~Granulator()
{
}

void Granulator::prepare(double sampleRate, int numChannels, juce::int64 lookaheadSamples, int maxGrains)
{
    mSampleRate       = sampleRate;
    mNumChannels      = numChannels;
    mLookaheadSamples = lookaheadSamples;
    mMaxGrains        = maxGrains;

    // Configure the shared window with Tukey shape for smooth grain edges
    // Window size set to accommodate maximum expected grain size (2048 samples covers pitch down to ~22 Hz at 44.1kHz)
    // Period will be set per-grain based on actual grain size in Grain::process()
    const int maxWindowSize = 2048;
    mWindow.setSizeShapePeriod(maxWindowSize, Window::Shape::kTukey, maxWindowSize);

    // Pre-allocate grain pool — all grains share the same window reference
    mGrains.clear();
    mGrains.reserve(maxGrains);

    for (int i = 0; i < maxGrains; ++i)
    {
        mGrains.emplace_back(SynthMark(), mWindow, mSourceBuffer, mLookaheadSamples);
    }
}

void Granulator::generateGrains(const std::vector<SynthMark>& synthMarks)
{
    int synthMarkIndex = 0;

    for (auto& grain : mGrains)
    {
        if (synthMarkIndex >= static_cast<int>(synthMarks.size()))
            break;

        // Only reuse grains that are finished or invalid
        if (!grain.isValid() || grain.isFinished())
        {
            grain.setGrain(synthMarks[synthMarkIndex], mLookaheadSamples);
            ++synthMarkIndex;
        }
    }

    jassert(synthMarkIndex == static_cast<int>(synthMarks.size()) &&
            "Not enough available grains - increase maxGrains or implement voice stealing");
}

void Granulator::process(juce::AudioBuffer<float>& outputBuffer, juce::int64 blockStartSample, juce::int64 blockEndSample)
{
    for (auto& grain : mGrains)
    {
        grain.process(outputBuffer, blockStartSample, blockEndSample);
    }
}

void Granulator::reset()
{
    for (auto& grain : mGrains)
    {
        grain.setGrain(SynthMark(), mLookaheadSamples);
    }
}
