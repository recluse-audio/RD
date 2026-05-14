#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/PULSAR/PulsarProcessor.h"

class PulsarEditor : public juce::AudioProcessorEditor
{
public:
    explicit PulsarEditor (PulsarProcessor& processor);
    ~PulsarEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PulsarProcessor& mProcessor;

    juce::Slider mFundamentalFreqSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
    juce::Label  mFundamentalFreqLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mFundamentalFreqAttachment;

    juce::Slider mFormantFreqSlider     { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
    juce::Label  mFormantFreqLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mFormantFreqAttachment;

    juce::Slider mWavePositionSlider    { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
    juce::Label  mWavePositionLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mWavePositionAttachment;

    juce::Slider mGainSlider            { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
    juce::Label  mGainLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mGainAttachment;

    juce::ToggleButton mOnOffButton { "Pulsar On/Off" };
    juce::AudioProcessorValueTreeState::ButtonAttachment mOnOffAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulsarEditor)
};
