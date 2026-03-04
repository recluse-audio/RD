#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/GAIN/GainProcessor.h"

class ProcessorControlsView : public juce::Component
{
public:
    explicit ProcessorControlsView (GainProcessor& gainProcessor);
    ~ProcessorControlsView() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    // Slider must be declared before the attachment
    juce::Slider mGainSlider;
    juce::Label  mGainLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorControlsView)
};
