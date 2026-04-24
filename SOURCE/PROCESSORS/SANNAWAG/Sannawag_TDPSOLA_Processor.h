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
#include "PROCESSORS/BASE/RD_Processor.h"
#include "TD_PSOLA.h"

class Sannawag_TDPSOLA_Processor : public RD_Processor
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

    //==============================================================================
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    //==============================================================================
    TD_PSOLA::TDPSOLA&                  getEngine() { return mEngine; }
    TD_PSOLA::TDPSOLA::Config&          getConfig() { return mConfig; }

    juce::int64 getAbsoluteSampleCount() const { return mAbsoluteSampleCount; }
    float       getShiftRatio()          const { return mShiftRatio.get(); }

    /** Offline one-shot render. Safe to call from a non-audio thread. */
    bool renderOffline (const juce::AudioBuffer<float>& input,
                        juce::AudioBuffer<float>&       output,
                        float                           shiftRatio);

    juce::AudioProcessorValueTreeState& getAPVTS() override { return mAPVTS; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    TD_PSOLA::TDPSOLA          mEngine;
    TD_PSOLA::TDPSOLA::Config  mConfig;
    juce::AudioBuffer<float>   mScratchBuffer;

    // Different than mBaseAPVTS in RD_Processor
    juce::AudioProcessorValueTreeState mAPVTS;

    juce::int64         mAbsoluteSampleCount = 0;
    juce::Atomic<float> mShiftRatio          { 1.0f };
    bool                mWasPlaying          = false;

    bool _didTransportJustStop();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sannawag_TDPSOLA_Processor)
};
