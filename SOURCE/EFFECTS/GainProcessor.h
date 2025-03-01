/**
 * @file GainProcessor.h
 * @author Ryan Devens
 * @brief 
 * @version 0.1
 * @date 2025-02-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */


#pragma once
#include "Util/Juce_Header.h"

/**
 * @brief Processor with one gain param and an apvts.
 * 
 * Useful for gain adjustment, but also for testing and mocking.
 * 
 */
class GainProcessor : public juce::AudioProcessor
                      , public juce::AudioProcessorValueTreeState::Listener
{
public:
    GainProcessor();
    ~GainProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override { return true; }

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override          { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override                        { return true;   }

    const juce::String getName() const override            { return "Gain PlugIn"; }
    bool acceptsMidi() const override                      { return false; }
    bool producesMidi() const override                     { return false; }
    double getTailLengthSeconds() const override           { return 0; }

    //==============================================================================
    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const juce::String getProgramName (int) override             { return "None"; }
    void changeProgramName (int, const juce::String&) override   {}

    //
    void getStateInformation (juce::MemoryBlock& destData) override
    {

    }

    //
    void setStateInformation (const void* data, int sizeInBytes) override
    {

    }

    juce::AudioProcessorValueTreeState& getAPVTS();

   // AudioProcessorValueTreeState::Listener callback
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setGain(float newGain);
    const int getLastBlockSizeFromPrepareToPlay();
    const double getLastSampleRateFromPrepareToPlay();

private:
    int mBlockSize = 512;
    double mSampleRate = 44100;
    juce::Atomic<float> mGainValue;
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout _createParameterLayout();
    void _updateGainValue(float newValue);

    // cleanup ugly code in PluginProcessor's constructor
    juce::AudioProcessor::BusesProperties _getBusesProperties();


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainProcessor)

};
