/**
 * GrainShifterEditor.h
 * Created by Ryan Devens
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/GRAIN/GrainShifterProcessor.h"

class GrainShifterEditor : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit GrainShifterEditor (GrainShifterProcessor& processor);
    ~GrainShifterEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;

private:
    GrainShifterProcessor& mProcessor;

    juce::Slider mShiftRatioSlider  { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    juce::Label  mShiftRatioLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mShiftRatioAttachment;

    juce::Slider mThresholdSlider   { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    juce::Label  mThresholdLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mThresholdAttachment;

    juce::ComboBox mPitchWindowSizeBox;
    juce::Label    mPitchWindowSizeLabel;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment mPitchWindowSizeAttachment;

    juce::ComboBox mPitchHopSizeBox;
    juce::Label    mPitchHopSizeLabel;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment mPitchHopSizeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrainShifterEditor)
};
