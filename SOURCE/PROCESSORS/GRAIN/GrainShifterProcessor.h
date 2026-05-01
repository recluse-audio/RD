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
#include "PROCESSORS/BASE/RD_Processor.h"
#include "CircularBuffer.h"
#include "PITCH/PitchManager.h"
#include "Granulator.h"

class GrainShifterEditor;

class GrainShifterProcessor : public RD_Processor
{
public:
    GrainShifterProcessor();
    ~GrainShifterProcessor() override;

    //==============================================================================
    void doPrepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void doProcessBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "Grain Shifter"; }

    //==============================================================================
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    //==============================================================================
    CircularBuffer& getCircularBuffer() { return mCircularBuffer; }
    PitchManager&   getPitchManager()   { return mPitchManager; }
    Granulator&     getGranulator()     { return mGranulator; }

    juce::int64 getAbsoluteSampleCount() const { return mAbsoluteSampleCount; }
    float       getShiftRatio()          const { return mShiftRatio.get(); }

    juce::AudioProcessorValueTreeState& getAPVTS() override { return mAPVTS; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    // mCircularBuffer MUST be declared before mGranulator —
    // Granulator holds a reference to it and is initialised via member init list.
    CircularBuffer mCircularBuffer;
    PitchManager   mPitchManager;
    Granulator     mGranulator;

    // Different than mBaseAPVTS in RD_Processor
    juce::AudioProcessorValueTreeState mAPVTS;

    juce::int64         mAbsoluteSampleCount  = 0;
    int                 mDetectionSampleCount = 0;
    juce::Atomic<float> mShiftRatio           { 1.0f };
    bool                mWasPlaying           = false;
    float mLastDetectedPeriod = -1.f;
    int mGranulationSampleCount = 0;
    bool _didTransportJustStop();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainShifterProcessor)
};
