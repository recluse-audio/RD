#include "GainEditor.h"

GainEditor::GainEditor (GainProcessor& processor)
    : AudioProcessorEditor (processor)
    , mProcessor (processor)
    , mGainAttachment (processor.getAPVTS(), "gain", mGainSlider)
{
    setName ("GainEditor_Component");

    mGainLabel.setText ("Gain", juce::dontSendNotification);
    mGainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (mGainLabel);

    addAndMakeVisible (mGainSlider);

    setSize (120, 280);
}

GainEditor::~GainEditor() {}

void GainEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void GainEditor::resized()
{
    auto area = getLocalBounds().reduced (10);

    mGainLabel.setBounds (area.removeFromTop (24));
    mGainSlider.setBounds (area);
}
