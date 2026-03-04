#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/GAIN/GainProcessor.h"
#include "PROCESSORS/TDPSOLA/TDPSOLA_Processor.h"

class ProcessorControlsView : public juce::Component
{
public:
    ProcessorControlsView (GainProcessor& gainProcessor,
                           TDPSOLA_Processor& tdpsolaProcessor);
    ~ProcessorControlsView() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    // Sliders must be declared before their attachments
    juce::Slider mGainSlider;
    juce::Label  mGainLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mGainAttachment;

    juce::Slider mShiftRatioSlider;
    juce::Label  mShiftRatioLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mShiftRatioAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorControlsView)
};
