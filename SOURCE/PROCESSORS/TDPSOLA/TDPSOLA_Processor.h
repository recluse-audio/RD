/**
 * TDPSOLA_Processor.h
 * Created by Ryan Devens
 *
 * juce::AudioProcessor that performs TD-PSOLA pitch shifting in real-time.
 *
 * Owns and coordinates three components:
 *   - CircularBuffer    : stores all incoming audio; the single source of truth for audio data
 *   - PitchManager      : runs pitch detection and tracks pitch/synth marks in absolute time
 *   - TD_Granulator     : maintains a pool of TD_Grain objects that overlap-add into the output
 */

#pragma once
#include "Util/Juce_Header.h"
#include "CircularBuffer.h"
#include "PITCH/PitchManager.h"
#include "TD_Granulator.h"

class TDPSOLA_Editor;

class TDPSOLA_Processor : public juce::AudioProcessor
                        , public juce::AudioProcessorValueTreeState::Listener
{
public:
    TDPSOLA_Processor();
    ~TDPSOLA_Processor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    // AudioProcessor boilerplate
    const juce::String getName() const override                 { return "TDPSOLA Processor"; }
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
    // AudioProcessorValueTreeState::Listener
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    //==============================================================================
    // Accessors
    CircularBuffer& getCircularBuffer() { return mCircularBuffer; }
    PitchManager&   getPitchManager()   { return mPitchManager; }
    TD_Granulator&  getGranulator()     { return mGranulator; }

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    juce::int64 getAbsoluteSampleCount() const { return mAbsoluteSampleCount; }
    float       getShiftRatio()          const { return mShiftRatio.get(); }

private:
    // mCircularBuffer MUST be declared before mGranulator —
    // TD_Granulator holds a reference to it and is initialised via member init list.
    CircularBuffer mCircularBuffer;
    PitchManager   mPitchManager;
    TD_Granulator  mGranulator;

    // apvts must come after the audio components so _createParameterLayout() is safe to call.
    juce::AudioProcessorValueTreeState apvts;

    double            mSampleRate            = 44100.0;
    int               mBlockSize             = 512;
    juce::int64       mAbsoluteSampleCount   = 0;
    int               mDetectionSampleCount  = 0;
    juce::Atomic<float> mShiftRatio          { 1.0f };

    juce::AudioProcessorValueTreeState::ParameterLayout _createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TDPSOLA_Processor)
};
