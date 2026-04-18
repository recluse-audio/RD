/**
 * GrainShifterProcessor.h
 * Created by Ryan Devens
 *
 * juce::AudioProcessor that performs TD-PSOLA pitch shifting in real-time.
 *
 * Owns and coordinates three components:
 *   - CircularBuffer  : stores all incoming audio; the single source of truth for audio data
 *   - PitchManager    : runs pitch detection and tracks pitch/synth marks in absolute time
 *   - Granulator      : maintains a pool of Grain objects that overlap-add into the output
 */

#pragma once
#include "Util/Juce_Header.h"
#include "CircularBuffer.h"
#include "PITCH/PitchManager.h"
#include "Granulator.h"

class GrainShifterEditor;

class GrainShifterProcessor : public juce::AudioProcessor
                            , public juce::AudioProcessorValueTreeState::Listener
{
public:
    GrainShifterProcessor();
    ~GrainShifterProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override                 { return "Grain Shifter"; }
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
    CircularBuffer& getCircularBuffer() { return mCircularBuffer; }
    PitchManager&   getPitchManager()   { return mPitchManager; }
    Granulator&     getGranulator()     { return mGranulator; }

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    juce::int64 getAbsoluteSampleCount() const { return mAbsoluteSampleCount; }
    float       getShiftRatio()          const { return mShiftRatio.get(); }

private:
    // mCircularBuffer MUST be declared before mGranulator —
    // Granulator holds a reference to it and is initialised via member init list.
    CircularBuffer mCircularBuffer;
    PitchManager   mPitchManager;
    Granulator     mGranulator;

    // apvts must come after the audio components so _createParameterLayout() is safe to call.
    juce::AudioProcessorValueTreeState apvts;

    double              mSampleRate           = 44100.0;
    int                 mBlockSize            = 512;
    juce::int64         mAbsoluteSampleCount  = 0;
    int                 mDetectionSampleCount = 0;
    juce::Atomic<float> mShiftRatio           { 1.0f };
    bool                mWasPlaying           = false;

    bool _didTransportJustStop();
    juce::AudioProcessorValueTreeState::ParameterLayout _createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainShifterProcessor)
};
