#include "RD_PluginProcessor.h"
#include "EDITORS/RD_PluginEditor.h"

RD_PluginProcessor::RD_PluginProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

RD_PluginProcessor::~RD_PluginProcessor() {}

void RD_PluginProcessor::prepareToPlay (double /*sampleRate*/, int /*samplesPerBlock*/) {}

void RD_PluginProcessor::releaseResources() {}

bool RD_PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void RD_PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (buffer);
}

bool RD_PluginProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* RD_PluginProcessor::createEditor()
{
    return new RD_PluginEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RD_PluginProcessor();
}
