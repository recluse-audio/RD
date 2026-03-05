/**
 * TDPSOLA_Editor.cpp
 * Created by Ryan Devens
 */

#include "TDPSOLA_Editor.h"

TDPSOLA_Editor::TDPSOLA_Editor (TDPSOLA_Processor& processor)
    : AudioProcessorEditor (processor)
    , mProcessor (processor)
    , mShiftRatioAttachment (processor.getAPVTS(), "shift_ratio", mShiftRatioSlider)
{
    setName ("TDPSOLA_Editor_Component");

    mShiftRatioLabel.setText ("Shift Ratio", juce::dontSendNotification);
    mShiftRatioLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (mShiftRatioLabel);

    addAndMakeVisible (mShiftRatioSlider);

    setSize (120, 280);
}

TDPSOLA_Editor::~TDPSOLA_Editor() {}

void TDPSOLA_Editor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void TDPSOLA_Editor::resized()
{
    auto area = getLocalBounds().reduced (10);

    mShiftRatioLabel.setBounds (area.removeFromTop (24));
    mShiftRatioSlider.setBounds (area);
}
