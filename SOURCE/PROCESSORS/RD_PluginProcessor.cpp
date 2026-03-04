#include "RD_PluginProcessor.h"
#include "EDITORS/RD_PluginEditor.h"

RD_PluginProcessor::RD_PluginProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    _buildGraph();
}

RD_PluginProcessor::~RD_PluginProcessor() {}

void RD_PluginProcessor::_buildGraph()
{
    mGraph.clear();

    mGraph.setBusesLayout ({ { juce::AudioChannelSet::stereo() },
                             { juce::AudioChannelSet::stereo() } });

    mAudioInputNode  = mGraph.addNode (std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor> (
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));

    mAudioOutputNode = mGraph.addNode (std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor> (
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

    mGainNode = mGraph.addNode (std::make_unique<GainProcessor>());

    for (int ch = 0; ch < 2; ++ch)
    {
        mGraph.addConnection ({{ mAudioInputNode->nodeID,  ch }, { mGainNode->nodeID,        ch }});
        mGraph.addConnection ({{ mGainNode->nodeID,        ch }, { mAudioOutputNode->nodeID, ch }});
    }
}

void RD_PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mGraph.enableAllBuses();

    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add  (juce::AudioChannelSet::stereo());
    layout.outputBuses.add (juce::AudioChannelSet::stereo());
    mGraph.setBusesLayout (layout);

    mGraph.setPlayConfigDetails (2, 2, sampleRate, samplesPerBlock);
    mGraph.prepareToPlay (sampleRate, samplesPerBlock);
}

void RD_PluginProcessor::releaseResources()
{
    mGraph.releaseResources();
}

bool RD_PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void RD_PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    mGraph.processBlock (buffer, midiMessages);
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
