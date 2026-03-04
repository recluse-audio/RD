#include "RD_PluginEditor.h"

RD_PluginEditor::RD_PluginEditor (RD_PluginProcessor& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    auto* gain    = processorRef.getGainProcessor();
    auto* tdpsola = processorRef.getTDPSOLAProcessor();

    if (gain != nullptr && tdpsola != nullptr)
    {
        mControlsView = std::make_unique<ProcessorControlsView> (*gain, *tdpsola);
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
