/**
 * TDPSOLA_Editor.cpp
 * Created by Ryan Devens
 */

#include "TDPSOLA_Editor.h"

TDPSOLA_Editor::TDPSOLA_Editor (TDPSOLA_Processor& processor)
    : AudioProcessorEditor (processor)
    , mProcessor (processor)
    , mShiftRatioAttachment (processor.getAPVTS(), "shift_ratio",       mShiftRatioSlider)
    , mThresholdAttachment  (processor.getAPVTS(), "pitch_threshold",   mThresholdSlider)
{
    setName ("TDPSOLA_Editor_Component");

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

TDPSOLA_Editor::~TDPSOLA_Editor() {}

void TDPSOLA_Editor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void TDPSOLA_Editor::resized()
{
    auto area = getLocalBounds().reduced (10);

    auto leftCol  = area.removeFromLeft (area.getWidth() / 2).reduced (4, 0);
    auto rightCol = area.reduced (4, 0);

    mShiftRatioLabel.setBounds (leftCol.removeFromTop (24));
    mShiftRatioSlider.setBounds (leftCol);

    mThresholdLabel.setBounds (rightCol.removeFromTop (24));
    mThresholdSlider.setBounds (rightCol);
}
