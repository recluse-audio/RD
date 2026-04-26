#include "RD_ProcessorSwapper.h"
#include "EDITORS/RD_ProcessorSwapperEditor.h"

RD_ProcessorSwapper::RD_ProcessorSwapper()
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
    mGrainShifterNodeID = mGraph.addNode (std::make_unique<GrainShifterProcessor>())->nodeID;

    _applyProcessorSwap();
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
    else if (index == ProcessorIndex::kGrainShifter)
    {
        for (auto* node : mGraph.getNodes())
            if (auto* p = dynamic_cast<GrainShifterProcessor*> (node->getProcessor()))
                return p;
    }
    return nullptr;
}

juce::AudioProcessor* RD_ProcessorSwapper::getActiveProcessor()
{
    return getProcessorByIndex (mActiveProcessorIndex);
}

//==============================================================================
void RD_ProcessorSwapper::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    RD_Processor::prepareToPlay (sampleRate, samplesPerBlock);

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
    RD_Processor::releaseResources();
    mGraph.releaseResources();
}

bool RD_ProcessorSwapper::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void RD_ProcessorSwapper::doProcessBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    mGraph.processBlock (buffer, midiMessages);

    const auto gainValue = mGainValue.get();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
            buffer.setSample (ch, sampleIndex, buffer.getSample (ch, sampleIndex) * gainValue);

    // if (mFade.getCurrentState() == Fade::FadeState::kFadingIn ||
    //     mFade.getCurrentState() == Fade::FadeState::kFadingOut)
    // {
    //     const float fadeValue = static_cast<float> (mFade.getCurrentFadeValue());
    //     for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    //         buffer.applyGain (ch, 0, buffer.getNumSamples(), fadeValue);
    //     mFade.incrementFadeValue (buffer.getNumSamples());
    // }

    // if (mFade.getCurrentState() == Fade::FadeState::kFullFade)
    // {
    //     _applyProcessorSwap();
    // }
}

//==============================================================================
bool RD_ProcessorSwapper::hasEditor() const { return true; }

juce::AudioProcessorEditor* RD_ProcessorSwapper::createEditor()
{
    return new RD_ProcessorSwapperEditor (*this);
}

#if BUILD_AS_PLUGIN
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RD_ProcessorSwapper();
}
#endif

//==============================================================================
void RD_ProcessorSwapper::setActiveProcessor (ProcessorIndex index)
{
    if (index != mActiveProcessorIndex)
    {
        mActiveProcessorIndex = index;
        _applyProcessorSwap();
    }
}

void RD_ProcessorSwapper::_applyProcessorSwap()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        mGraph.removeConnection ({{ mAudioInputNodeID, ch }, { mGainNodeID,         ch }});
        mGraph.removeConnection ({{ mAudioInputNodeID, ch }, { mGrainShifterNodeID, ch }});
        mGraph.removeConnection ({{ mGainNodeID,         ch }, { mAudioOutputNodeID, ch }});
        mGraph.removeConnection ({{ mGrainShifterNodeID, ch }, { mAudioOutputNodeID, ch }});
    }

    auto activeNodeID = (mActiveProcessorIndex == ProcessorIndex::kGain) ? mGainNodeID : mGrainShifterNodeID;

    for (int ch = 0; ch < 2; ++ch)
    {
        mGraph.addConnection ({{ mAudioInputNodeID, ch }, { activeNodeID,        ch }});
        mGraph.addConnection ({{ activeNodeID,      ch }, { mAudioOutputNodeID,  ch }});
    }
}
