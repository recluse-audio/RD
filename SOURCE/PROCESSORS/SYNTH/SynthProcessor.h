/**
 * Created on 2026-05-13 by Ryan Devens with Peace and Love
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/BASE/RD_Processor.h"
#include <memory>

namespace rd_dsp { class Synth; }

namespace SynthParams
{
    static const juce::String kWavePositionID = "synth_wave_position";
    static const juce::String kGainID         = "synth_gain";
    static constexpr int      kDefaultNumVoices = 4;
}

class SynthProcessor : public RD_Processor
{
public:
    SynthProcessor();
    ~SynthProcessor() override;

    void doPrepareToPlay (double sampleRate, int maxBlockSize) override;
    void doProcessBlock  (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    juce::AudioProcessorValueTreeState& getAPVTS() override { return mAPVTS; }

    const juce::String getName() const override { return "Synth Processor"; }

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Thread-safe MIDI ingress. UI thread pushes note on/off; audio thread drains in processBlock. */
    juce::MidiMessageCollector& getMidiCollector() noexcept { return mMidiCollector; }

private:
    std::unique_ptr<rd_dsp::Synth> mSynth;

    juce::AudioProcessorValueTreeState mAPVTS;

    juce::MidiMessageCollector mMidiCollector;

    juce::Atomic<float> mGain { 0.3f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthProcessor)
};
