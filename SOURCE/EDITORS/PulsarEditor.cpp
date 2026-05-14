#include "PulsarEditor.h"

PulsarEditor::PulsarEditor (PulsarProcessor& processor)
    : AudioProcessorEditor (processor)
    , mProcessor (processor)
    , mFundamentalFreqAttachment (processor.getAPVTS(), PulsarParams::kFundamentalFreqID, mFundamentalFreqSlider)
    , mFormantFreqAttachment     (processor.getAPVTS(), PulsarParams::kFormantFreqID,     mFormantFreqSlider)
    , mWavePositionAttachment    (processor.getAPVTS(), PulsarParams::kWavePositionID,    mWavePositionSlider)
    , mGainAttachment            (processor.getAPVTS(), PulsarParams::kGainID,            mGainSlider)
    , mOnOffAttachment           (processor.getAPVTS(), PulsarParams::kOnOffID,           mOnOffButton)
{
    addAndMakeVisible (mOnOffButton);
    setName ("PulsarEditor_Component");

    auto setupLabeledSlider = [this] (juce::Label& label, juce::Slider& slider, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (label);
        addAndMakeVisible (slider);
    };

    setupLabeledSlider (mFundamentalFreqLabel, mFundamentalFreqSlider, "Fundamental Frequency (Hz)");
    setupLabeledSlider (mFormantFreqLabel,     mFormantFreqSlider,     "Formant Frequency (Hz)");
    setupLabeledSlider (mWavePositionLabel,    mWavePositionSlider,    "Wave Position");
    setupLabeledSlider (mGainLabel,            mGainSlider,            "Gain");

    setSize (360, 500);
}

PulsarEditor::~PulsarEditor() {}

void PulsarEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void PulsarEditor::resized()
{
    auto area = getLocalBounds().reduced (10);

    mOnOffButton.setBounds (area.removeFromTop (28));
    area.removeFromTop (8);

    auto layoutOne = [&area] (juce::Label& label, juce::Slider& slider)
    {
        label .setBounds (area.removeFromTop (20));
        slider.setBounds (area.removeFromTop (60));
        area.removeFromTop (8);
    };

    layoutOne (mFundamentalFreqLabel, mFundamentalFreqSlider);
    layoutOne (mFormantFreqLabel,     mFormantFreqSlider);
    layoutOne (mWavePositionLabel,    mWavePositionSlider);
    layoutOne (mGainLabel,            mGainSlider);
}
