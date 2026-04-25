/**
 * RD_Processor.h
 * Created by Ryan Devens on 2026-04-23 (with love ofc)
 *
 * Base class from which all my processors shall inherit until I regret doing so.
 *
 * Owns `mBaseAPVTS` (a default APVTS carrying a "gain" parameter) so that
 * RD_Processor is not abstract and can be instantiated on its own.
 *
 * Derived classes declare their own `mAPVTS` and override getAPVTS() to
 * return a reference to it.
 */

#pragma once
#include "Util/Juce_Header.h"
#include "../../DATA_LOGGER/DataLogger.h"

class RD_Processor : public juce::AudioProcessor
                   , public juce::AudioProcessorValueTreeState::Listener
                   , public DataLogger
{
public:
    RD_Processor();
    ~RD_Processor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //==============================================================================
    const juce::String getName() const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    const double getLastSampleRateFromPrepareToPlay() const;
    const int    getLastBlockSizeFromPrepareToPlay()  const;

    //==============================================================================
    /** Returns the APVTS for this processor. Derived classes override to
        return a reference to their own `mAPVTS`. */
    virtual juce::AudioProcessorValueTreeState& getAPVTS();

    juce::File createProcessorDataLogFile();
    juce::File createProcessBlockDataLogFile(juce::AudioBuffer<float> processBuffer, bool isPreProcessing = true);
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void setGain (float newGain);

protected:
    double mSampleRate = 44100.0;
    int    mBlockSize  = 512;

    juce::Atomic<float>                mGainValue;
    juce::AudioProcessorValueTreeState mBaseAPVTS;

private:
    static BusesProperties _getDefaultBusesProperties();
    static juce::AudioProcessorValueTreeState::ParameterLayout _createParameterLayout();
    void _updateGainValue (float newValue);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RD_Processor)
};
