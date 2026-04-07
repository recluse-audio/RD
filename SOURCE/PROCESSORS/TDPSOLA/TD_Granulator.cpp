/**
 * TD_Granulator.cpp
 * Created by Ryan Devens
 *
 * Implementation of TD_Granulator.
 */

#include "TD_Granulator.h"

TD_Granulator::TD_Granulator(CircularBuffer& sourceBuffer)
    : mSourceBuffer(sourceBuffer)
{
}

TD_Granulator::~TD_Granulator()
{
}

void TD_Granulator::prepare(double sampleRate, int numChannels, juce::int64 lookaheadSamples, int maxGrains)
{
    mSampleRate = sampleRate;
    mNumChannels = numChannels;
    mLookaheadSamples = lookaheadSamples;
    mMaxGrains = maxGrains;

    // Configure the shared window with Tukey shape for smooth grain edges
    // Window size set to accommodate maximum expected grain size (2048 samples covers pitch down to ~22 Hz at 44.1kHz)
    // Period will be set per-grain based on actual grain size in TD_Grain::process()
    const int maxWindowSize = 2048;
    mWindow.setSizeShapePeriod(maxWindowSize, Window::Shape::kTukey, maxWindowSize);

    // Pre-allocate grains pool
    // All grains share the same window reference
    mGrains.clear();
    mGrains.reserve(maxGrains);

    // Create the grain pool with invalid grains (all referencing mWindow and mSourceBuffer)
    for (int i = 0; i < maxGrains; ++i)
    {
        mGrains.emplace_back(SynthMark(), mWindow, mSourceBuffer, mLookaheadSamples);
    }
}

void TD_Granulator::generateGrains(const std::vector<SynthMark>& synthMarks)
{
    int synthMarkIndex = 0;

    for (auto& grain : mGrains)
    {
        // If we've assigned all synth marks, we're done
        if (synthMarkIndex >= static_cast<int>(synthMarks.size()))
            break;

        // Only reuse grains that are finished or invalid
        if (!grain.isValid() || grain.isFinished())
        {
            // Reuse this grain for the next synth mark
            grain.setGrain(synthMarks[synthMarkIndex], mLookaheadSamples);
            ++synthMarkIndex;
        }
    }

    // If we couldn't assign all synth marks, we've run out of available grains
    // This is a voice stealing scenario - could implement more sophisticated logic here
    jassert(synthMarkIndex == static_cast<int>(synthMarks.size()) &&
            "Not enough available grains - increase maxGrains or implement voice stealing");
}

void TD_Granulator::process(juce::AudioBuffer<float>& outputBuffer, juce::int64 blockStartSample, juce::int64 blockEndSample)
{
    // Process each grain - each grain handles its own overlap calculation and synthesis
    for (auto& grain : mGrains)
    {
        grain.process(outputBuffer, blockStartSample, blockEndSample);
    }
}

void TD_Granulator::reset()
{
    // Reset all grains to make them invalid/available
    // Don't clear the array - keep the pre-allocated pool
    for (auto& grain : mGrains)
    {
        // Reset to invalid state (empty SynthMark)
        grain.setGrain(SynthMark(), mLookaheadSamples);
    }
}
