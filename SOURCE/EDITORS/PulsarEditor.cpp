#include "PulsarEditor.h"

PulsarEditor::PulsarEditor (PulsarProcessor& processor)
    : AudioProcessorEditor (processor)
    , mProcessor (processor)
    , mRateAttachment           (processor.getAPVTS(), PulsarParams::kRateID,           mRateSlider)
    , mGrainFrequencyAttachment (processor.getAPVTS(), PulsarParams::kGrainFrequencyID, mGrainFrequencySlider)
    , mGrainCyclesAttachment    (processor.getAPVTS(), PulsarParams::kGrainCyclesID,    mGrainCyclesSlider)
    , mWavePositionAttachment   (processor.getAPVTS(), PulsarParams::kWavePositionID,   mWavePositionSlider)
    , mGainAttachment           (processor.getAPVTS(), PulsarParams::kGainID,           mGainSlider)
{
    setName ("PulsarEditor_Component");

    auto setupLabeledSlider = [this] (juce::Label& label, juce::Slider& slider, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (label);
        addAndMakeVisible (slider);
    };

    setupLabeledSlider (mRateLabel,           mRateSlider,           "Rate (Hz)");
    setupLabeledSlider (mGrainFrequencyLabel, mGrainFrequencySlider, "Grain Frequency (Hz)");
    setupLabeledSlider (mGrainCyclesLabel,    mGrainCyclesSlider,    "Grain Cycles");
    setupLabeledSlider (mWavePositionLabel,   mWavePositionSlider,   "Wave Position");
    setupLabeledSlider (mGainLabel,           mGainSlider,           "Gain");

    setSize (360, 540);
}

PulsarEditor::~PulsarEditor() {}

void PulsarEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void PulsarEditor::resized()
{
    auto area = getLocalBounds().reduced (10);

    auto layoutOne = [&area] (juce::Label& label, juce::Slider& slider)
    {
        label .setBounds (area.removeFromTop (20));
        slider.setBounds (area.removeFromTop (60));
        area.removeFromTop (8);
    };

    layoutOne (mRateLabel,           mRateSlider);
    layoutOne (mGrainFrequencyLabel, mGrainFrequencySlider);
    layoutOne (mGrainCyclesLabel,    mGrainCyclesSlider);
    layoutOne (mWavePositionLabel,   mWavePositionSlider);
    layoutOne (mGainLabel,           mGainSlider);
}
