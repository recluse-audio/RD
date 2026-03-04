#include "RD_PluginEditor.h"

RD_PluginEditor::RD_PluginEditor (RD_PluginProcessor& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    setSize (400, 300);
}

RD_PluginEditor::~RD_PluginEditor() {}

void RD_PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (16.0f));
    g.drawFittedText ("RD Plugin", getLocalBounds(), juce::Justification::centred, 1);
}

void RD_PluginEditor::resized() {}
