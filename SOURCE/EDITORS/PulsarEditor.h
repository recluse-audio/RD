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

    juce::Slider mRateSlider           { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
    juce::Label  mRateLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mRateAttachment;

    juce::Slider mGrainFrequencySlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
    juce::Label  mGrainFrequencyLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mGrainFrequencyAttachment;

    juce::Slider mGrainCyclesSlider    { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
    juce::Label  mGrainCyclesLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mGrainCyclesAttachment;

    juce::Slider mWavePositionSlider   { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
    juce::Label  mWavePositionLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mWavePositionAttachment;

    juce::Slider mGainSlider           { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
    juce::Label  mGainLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulsarEditor)
};
