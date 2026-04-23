/**
 * Sannawag_TDPSOLA_Processor.h
 * Created by Ryan Devens
 *
 * juce::AudioProcessor wrapper around the offline TD_PSOLA::TDPSOLA engine.
 *
 * NOTE: TD_PSOLA::TDPSOLA is an offline algorithm (analyses the entire buffer
 * up front). Driving it from processBlock() re-runs pitch detection on every
 * block, which is only viable for non-realtime rendering (e.g. the file
 * processing path). Real-time use will produce artefacts at block boundaries.
 */

#pragma once
#include "Util/Juce_Header.h"
#include "TD_PSOLA.h"

class Sannawag_TDPSOLA_Processor : public juce::AudioProcessor
                                 , public juce::AudioProcessorValueTreeState::Listener
{
public:
    Sannawag_TDPSOLA_Processor();
    ~Sannawag_TDPSOLA_Processor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return false; }

    //==============================================================================
    const juce::String getName() const override                 { return "Sannawag TD-PSOLA"; }
    bool acceptsMidi() const override                           { return false; }
    bool producesMidi() const override                          { return false; }
    double getTailLengthSeconds() const override                { return 0.0; }
    int getNumPrograms() override                               { return 1; }
    int getCurrentProgram() override                            { return 0; }
    void setCurrentProgram (int) override                       {}
    const juce::String getProgramName (int) override            { return "None"; }
    void changeProgramName (int, const juce::String&) override  {}
    void getStateInformation (juce::MemoryBlock& d) override    { juce::ignoreUnused(d); }
    void setStateInformation (const void* d, int n) override    { juce::ignoreUnused(d, n); }

    //==============================================================================
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getAPVTS()  { return apvts; }
    TD_PSOLA::TDPSOLA&                  getEngine() { return mEngine; }
    TD_PSOLA::TDPSOLA::Config&          getConfig() { return mConfig; }

    juce::int64 getAbsoluteSampleCount() const { return mAbsoluteSampleCount; }
    float       getShiftRatio()          const { return mShiftRatio.get(); }

    /** Offline one-shot render. Safe to call from a non-audio thread. */
    bool renderOffline (const juce::AudioBuffer<float>& input,
                        juce::AudioBuffer<float>&       output,
                        float                           shiftRatio);

private:
    TD_PSOLA::TDPSOLA          mEngine;
    TD_PSOLA::TDPSOLA::Config  mConfig;
    juce::AudioBuffer<float>   mScratchBuffer;

    juce::AudioProcessorValueTreeState apvts;

    double              mSampleRate          = 44100.0;
    int                 mBlockSize           = 512;
    juce::int64         mAbsoluteSampleCount = 0;
    juce::Atomic<float> mShiftRatio          { 1.0f };
    bool                mWasPlaying          = false;

    bool _didTransportJustStop();
    juce::AudioProcessorValueTreeState::ParameterLayout _createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sannawag_TDPSOLA_Processor)
};
