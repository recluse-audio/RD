#include "ProcessorControlsView.h"

ProcessorControlsView::ProcessorControlsView (GainProcessor& gainProcessor)
    : mGainAttachment (gainProcessor.getAPVTS(), "gain", mGainSlider)
{
    mGainLabel.setText ("Gain", juce::dontSendNotification);
    mGainLabel.attachToComponent (&mGainSlider, true);

    mGainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    mGainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);

    addAndMakeVisible (mGainSlider);
    addAndMakeVisible (mGainLabel);
}

ProcessorControlsView::~ProcessorControlsView() {}

void ProcessorControlsView::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey.darker());
}

void ProcessorControlsView::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromLeft (50); // space for label
    mGainSlider.setBounds (area);
}
