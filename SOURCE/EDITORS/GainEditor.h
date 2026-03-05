#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/GAIN/GainProcessor.h"

class GainEditor : public juce::AudioProcessorEditor
{
public:
    explicit GainEditor (GainProcessor& processor);
    ~GainEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    GainProcessor& mProcessor;

    juce::Slider mGainSlider { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    juce::Label  mGainLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment mGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainEditor)
};
