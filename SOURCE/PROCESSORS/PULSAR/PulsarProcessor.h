/**
 * Created on 2026-05-13 by Ryan Devens with Peace and Love
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/BASE/RD_Processor.h"
#include <memory>

namespace rd_dsp { class PulsarTrain; }

namespace PulsarParams
{
    static const juce::String kFundamentalFreqID = "pulsar_fundamental_frequency";
    static const juce::String kFormantFreqID     = "pulsar_formant_frequency";
    static const juce::String kWavePositionID    = "pulsar_wave_position";
    static const juce::String kGainID            = "pulsar_gain";
    static const juce::String kOnOffID           = "pulsar_on_off";
}

class PulsarProcessor : public RD_Processor
{
public:
    PulsarProcessor();
    ~PulsarProcessor() override;

    void doPrepareToPlay (double sampleRate, int maxBlockSize) override;
    void doProcessBlock  (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    juce::AudioProcessorValueTreeState& getAPVTS() override { return mAPVTS; }

    const juce::String getName() const override { return "Pulsar Processor"; }

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    std::unique_ptr<rd_dsp::PulsarTrain> mPulsarTrain;

    juce::AudioProcessorValueTreeState mAPVTS;

    juce::Atomic<float> mGain { 0.3f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulsarProcessor)
};
