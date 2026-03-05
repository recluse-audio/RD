/**
 * TDPSOLA_Editor.h
 * Created by Ryan Devens
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/TDPSOLA/TDPSOLA_Processor.h"

class TDPSOLA_Editor : public juce::AudioProcessorEditor
{
public:
    explicit TDPSOLA_Editor (TDPSOLA_Processor& processor);
    ~TDPSOLA_Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    TDPSOLA_Processor& mProcessor;

    juce::Slider mShiftRatioSlider { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    juce::Label  mShiftRatioLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment mShiftRatioAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TDPSOLA_Editor)
};
