/**
 * Created on 2026-05-08 by Ryan Devens with Peace and Love
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/BASE/RD_Processor.h"
#include <memory>

namespace rd_dsp { class Oscillator; class Wavetable; }

namespace OscillatorParams
{
    static const juce::String kOnOffID     = "oscillator_on_off";
    static const juce::String kFrequencyID = "oscillator_frequency";
    static const juce::String kGainID      = "oscillator_gain";
}

class OscillatorProcessor : public RD_Processor
{
public:
    OscillatorProcessor();
    ~OscillatorProcessor() override;

    void doPrepareToPlay(double sampleRate, int maxBlockSize) override;
    void doProcessBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    juce::AudioProcessorValueTreeState& getAPVTS() override { return mAPVTS; }

    const juce::String getName() const override{ return "Oscillator Processor"; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    std::unique_ptr<rd_dsp::Wavetable>  mWavetable;
    std::unique_ptr<rd_dsp::Oscillator> mOscillator;

    // Owned APVTS for this processor (separate from RD_Processor::mBaseAPVTS).
    juce::AudioProcessorValueTreeState mAPVTS;

    juce::Atomic<float> mGain { 0.3f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorProcessor)
};
