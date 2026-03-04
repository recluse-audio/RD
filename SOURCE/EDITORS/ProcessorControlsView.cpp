#include "ProcessorControlsView.h"

ProcessorControlsView::ProcessorControlsView (GainProcessor& gainProcessor,
                                              TDPSOLA_Processor& tdpsolaProcessor)
    : mGainAttachment       (gainProcessor.getAPVTS(),    "gain",        mGainSlider)
    , mShiftRatioAttachment (tdpsolaProcessor.getAPVTS(), "shift_ratio", mShiftRatioSlider)
{
    mGainLabel.setText ("Gain", juce::dontSendNotification);
    mGainLabel.attachToComponent (&mGainSlider, true);
    mGainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    mGainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible (mGainSlider);
    addAndMakeVisible (mGainLabel);

    mShiftRatioLabel.setText ("Pitch", juce::dontSendNotification);
    mShiftRatioLabel.attachToComponent (&mShiftRatioSlider, true);
    mShiftRatioSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    mShiftRatioSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible (mShiftRatioSlider);
    addAndMakeVisible (mShiftRatioLabel);
}

ProcessorControlsView::~ProcessorControlsView() {}

void ProcessorControlsView::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey.darker());
}

void ProcessorControlsView::resized()
{
    auto area = getLocalBounds().reduced (8);
    const int labelWidth = 50;
    const int rowHeight  = area.getHeight() / 2;

    auto gainRow       = area.removeFromTop (rowHeight);
    auto shiftRatioRow = area;

    gainRow.removeFromLeft (labelWidth);
    mGainSlider.setBounds (gainRow);

    shiftRatioRow.removeFromLeft (labelWidth);
    mShiftRatioSlider.setBounds (shiftRatioRow);
}
