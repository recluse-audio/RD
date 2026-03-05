#include "RD_PluginEditor.h"

RD_PluginEditor::RD_PluginEditor (RD_ProcessorSwapper& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    auto* gain    = dynamic_cast<GainProcessor*>     (processorRef.getProcessorByIndex (RD_ProcessorSwapper::ProcessorIndex::kGain));
    auto* tdpsola = dynamic_cast<TDPSOLA_Processor*> (processorRef.getProcessorByIndex (RD_ProcessorSwapper::ProcessorIndex::kTDPSOLA));

    if (gain != nullptr && tdpsola != nullptr)
    {
        mControlsView = std::make_unique<ActiveProcessorView> (*gain, *tdpsola);
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
