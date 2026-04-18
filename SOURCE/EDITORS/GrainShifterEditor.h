/**
 * GrainShifterEditor.h
 * Created by Ryan Devens
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/GRAIN/GrainShifterProcessor.h"

class GrainShifterEditor : public juce::AudioProcessorEditor
{
public:
    explicit GrainShifterEditor (GrainShifterProcessor& processor);
    ~GrainShifterEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    GrainShifterProcessor& mProcessor;

    juce::Slider mShiftRatioSlider  { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    juce::Label  mShiftRatioLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mShiftRatioAttachment;

    juce::Slider mThresholdSlider   { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    juce::Label  mThresholdLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mThresholdAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainShifterEditor)
};
