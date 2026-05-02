/**
 * GrainShifterEditor.cpp
 * Created by Ryan Devens
 */

#include "GrainShifterEditor.h"

namespace
{
    void populateChoiceBox (juce::ComboBox& box,
                            juce::AudioProcessorValueTreeState& apvts,
                            const juce::String& paramID)
    {
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramID)))
        {
            box.clear (juce::dontSendNotification);
            int itemId = 1;
            for (const auto& choice : choiceParam->choices)
                box.addItem (choice, itemId++);
        }
    }
}

GrainShifterEditor::GrainShifterEditor (GrainShifterProcessor& processor)
    : AudioProcessorEditor (processor)
    , mProcessor (processor)
    , mShiftRatioAttachment (processor.getAPVTS(), "shift_ratio",     mShiftRatioSlider)
    , mThresholdAttachment  (processor.getAPVTS(), "pitch_threshold", mThresholdSlider)
    , mPitchWindowSizeAttachment ((populateChoiceBox (mPitchWindowSizeBox, processor.getAPVTS(), "pitch_window_size"),
                                   processor.getAPVTS()), "pitch_window_size", mPitchWindowSizeBox)
    , mPitchHopSizeAttachment    ((populateChoiceBox (mPitchHopSizeBox,    processor.getAPVTS(), "pitch_hop_size"),
                                   processor.getAPVTS()), "pitch_hop_size",    mPitchHopSizeBox)
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

    mPitchWindowSizeLabel.setText ("Pitch Window Size", juce::dontSendNotification);
    mPitchWindowSizeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (mPitchWindowSizeLabel);
    addAndMakeVisible (mPitchWindowSizeBox);

    mPitchHopSizeLabel.setText ("Pitch Hop Size", juce::dontSendNotification);
    mPitchHopSizeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (mPitchHopSizeLabel);
    addAndMakeVisible (mPitchHopSizeBox);

    setSize (360, 320);

    timerCallback();
    startTimerHz (10);
}

GrainShifterEditor::~GrainShifterEditor() {}

void GrainShifterEditor::timerCallback()
{
    const bool playing = mProcessor.isTransportPlaying();
    mPitchWindowSizeBox.setEnabled (! playing);
    mPitchWindowSizeLabel.setEnabled (! playing);
}

void GrainShifterEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void GrainShifterEditor::resized()
{
    auto area = getLocalBounds().reduced (10);

    auto bottomRow = area.removeFromBottom (60);

    auto leftCol  = area.removeFromLeft (area.getWidth() / 2).reduced (4, 0);
    auto rightCol = area.reduced (4, 0);

    mShiftRatioLabel.setBounds (leftCol.removeFromTop (24));
    mShiftRatioSlider.setBounds (leftCol);

    mThresholdLabel.setBounds (rightCol.removeFromTop (24));
    mThresholdSlider.setBounds (rightCol);

    auto windowCol = bottomRow.removeFromLeft (bottomRow.getWidth() / 2).reduced (4, 2);
    auto hopCol    = bottomRow.reduced (4, 2);

    mPitchWindowSizeLabel.setBounds (windowCol.removeFromTop (24));
    mPitchWindowSizeBox  .setBounds (windowCol);

    mPitchHopSizeLabel.setBounds (hopCol.removeFromTop (24));
    mPitchHopSizeBox  .setBounds (hopCol);
}
