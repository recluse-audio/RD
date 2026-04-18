/**
 * GrainShifterEditor.cpp
 * Created by Ryan Devens
 */

#include "GrainShifterEditor.h"

GrainShifterEditor::GrainShifterEditor (GrainShifterProcessor& processor)
    : AudioProcessorEditor (processor)
    , mProcessor (processor)
    , mShiftRatioAttachment (processor.getAPVTS(), "shift_ratio",     mShiftRatioSlider)
    , mThresholdAttachment  (processor.getAPVTS(), "pitch_threshold", mThresholdSlider)
{
    setName ("GrainShifterEditor_Component");

    mShiftRatioLabel.setText ("Shift Ratio", juce::dontSendNotification);
    mShiftRatioLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (mShiftRatioLabel);
    addAndMakeVisible (mShiftRatioSlider);

    mThresholdLabel.setText ("Pitch Threshold", juce::dontSendNotification);
    mThresholdLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (mThresholdLabel);
    addAndMakeVisible (mThresholdSlider);

    setSize (240, 280);
}

GrainShifterEditor::~GrainShifterEditor() {}

void GrainShifterEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void GrainShifterEditor::resized()
{
    auto area = getLocalBounds().reduced (10);

    auto leftCol  = area.removeFromLeft (area.getWidth() / 2).reduced (4, 0);
    auto rightCol = area.reduced (4, 0);

    mShiftRatioLabel.setBounds (leftCol.removeFromTop (24));
    mShiftRatioSlider.setBounds (leftCol);

    mThresholdLabel.setBounds (rightCol.removeFromTop (24));
    mThresholdSlider.setBounds (rightCol);
}
