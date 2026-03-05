#include "RD_ProcessorSwapper.h"
#include "EDITORS/RD_PluginEditor.h"

RD_ProcessorSwapper::RD_ProcessorSwapper()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    _buildGraph();
}

RD_ProcessorSwapper::~RD_ProcessorSwapper() {}

//==============================================================================
void RD_ProcessorSwapper::_buildGraph()
{
    mGraph.clear();

    mGraph.setBusesLayout ({ { juce::AudioChannelSet::stereo() },
                             { juce::AudioChannelSet::stereo() } });

    mAudioInputNodeID  = mGraph.addNode (std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor> (
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode))->nodeID;

    mAudioOutputNodeID = mGraph.addNode (std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor> (
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))->nodeID;

    mGainNodeID    = mGraph.addNode (std::make_unique<GainProcessor>())->nodeID;
    mTDPSOLANodeID = mGraph.addNode (std::make_unique<TDPSOLA_Processor>())->nodeID;

    // Series chain: Input → Gain → TDPSOLA → Output
    for (int ch = 0; ch < 2; ++ch)
    {
        mGraph.addConnection ({{ mAudioInputNodeID,  ch }, { mGainNodeID,        ch }});
        mGraph.addConnection ({{ mGainNodeID,        ch }, { mTDPSOLANodeID,     ch }});
        mGraph.addConnection ({{ mTDPSOLANodeID,     ch }, { mAudioOutputNodeID, ch }});
    }
}

//==============================================================================
juce::AudioProcessor* RD_ProcessorSwapper::getProcessorByIndex (ProcessorIndex index)
{
    if (index == ProcessorIndex::kGain)
    {
        for (auto* node : mGraph.getNodes())
            if (auto* p = dynamic_cast<GainProcessor*> (node->getProcessor()))
                return p;
    }
    else if (index == ProcessorIndex::kTDPSOLA)
    {
        for (auto* node : mGraph.getNodes())
            if (auto* p = dynamic_cast<TDPSOLA_Processor*> (node->getProcessor()))
                return p;
    }
    return nullptr;
}

juce::AudioProcessor* RD_ProcessorSwapper::getActiveProcessor()
{
    return getProcessorByIndex (mActiveProcessor);
}

//==============================================================================
void RD_ProcessorSwapper::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mGraph.enableAllBuses();

    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add  (juce::AudioChannelSet::stereo());
    layout.outputBuses.add (juce::AudioChannelSet::stereo());
    mGraph.setBusesLayout (layout);

    mGraph.setPlayConfigDetails (2, 2, sampleRate, samplesPerBlock);
    mGraph.prepareToPlay (sampleRate, samplesPerBlock);
}

void RD_ProcessorSwapper::releaseResources()
{
    mGraph.releaseResources();
}

bool RD_ProcessorSwapper::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void RD_ProcessorSwapper::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    mGraph.processBlock (buffer, midiMessages);
}

//==============================================================================
bool RD_ProcessorSwapper::hasEditor() const { return true; }

juce::AudioProcessorEditor* RD_ProcessorSwapper::createEditor()
{
    return new RD_PluginEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RD_ProcessorSwapper();
}
