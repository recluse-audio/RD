/**
 * RD_Processor.h
 * Created by Ryan Devens on 2026-04-23 (with love ofc)
 *
 * Base class from which all my processors shall inherit until I regret doing so.
 *
 * Absorbs the boilerplate that every juce::AudioProcessor in RD ends up writing
 * by hand: program/state no-ops, MIDI flags, tail length, sample-rate caching
 * from prepareToPlay, and a default stereo BusesProperties.
 *
 * Derived classes still own:
 *   - getName()
 *   - processBlock()
 *   - createEditor() / hasEditor()
 *   - any parameter/APVTS setup
 *   - prepareToPlay()/releaseResources() specifics (call base prepareToPlay to
 *     keep the cached sample rate and block size in sync)
 */

#pragma once
#include "Util/Juce_Header.h"

class RD_Processor : public juce::AudioProcessor
{
public:
    RD_Processor();
    explicit RD_Processor (const BusesProperties& busesProperties);
    ~RD_Processor() override = default;

    //==============================================================================
    // Caches sampleRate and samplesPerBlock. Override in derived classes and call
    // RD_Processor::prepareToPlay(sampleRate, samplesPerBlock) to keep the getters
    // below working.
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        juce::ignoreUnused (layouts);
        return true;
    }

    //==============================================================================
    // Boilerplate that every RD processor shares. Override in derived classes when
    // the defaults don't fit (e.g. hasEditor()).
    bool acceptsMidi() const override                           { return false; }
    bool producesMidi() const override                          { return false; }
    double getTailLengthSeconds() const override                { return 0.0; }

    int getNumPrograms() override                               { return 1; }
    int getCurrentProgram() override                            { return 0; }
    void setCurrentProgram (int) override                       {}
    const juce::String getProgramName (int) override            { return "None"; }
    void changeProgramName (int, const juce::String&) override  {}

    void getStateInformation (juce::MemoryBlock& destData) override
    {
        juce::ignoreUnused (destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        juce::ignoreUnused (data, sizeInBytes);
    }

    //==============================================================================
    // Sample rate / block size retrieval — the original motivation for this class.
    const double getLastSampleRateFromPrepareToPlay() const { return mSampleRate; }
    const int    getLastBlockSizeFromPrepareToPlay()  const { return mBlockSize; }

protected:
    double mSampleRate = 44100.0;
    int    mBlockSize  = 512;

private:
    static BusesProperties _getDefaultBusesProperties();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RD_Processor)
};
