#include "RD_PluginEditor.h"

RD_PluginEditor::RD_PluginEditor (RD_PluginProcessor& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    if (auto* gain = processorRef.getGainProcessor())
    {
        mControlsView = std::make_unique<ProcessorControlsView> (*gain);
        addAndMakeVisible (*mControlsView);
    }

    setSize (400, 300);
}

RD_PluginEditor::~RD_PluginEditor() {}

void RD_PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void RD_PluginEditor::resized()
{
    if (mControlsView)
        mControlsView->setBounds (getLocalBounds().reduced (20));
}
