#include "RD_ProcessorSwapper.h"
#include "EDITORS/RD_ProcessorSwapperEditor.h"

RD_ProcessorSwapper::RD_ProcessorSwapper()
{
    _buildGraph();

    for (int i = 0; i < getNumProcessors(); ++i)
    {
        auto* audioProc = getProcessorByIndex (static_cast<ProcessorIndex> (i));
        if (auto* rdProc = dynamic_cast<RD_Processor*> (audioProc))
            addChild (rdProc);
    }
}

RD_ProcessorSwapper::~RD_ProcessorSwapper()
{
    for (int i = 0; i < getNumProcessors(); ++i)
    {
        auto* audioProc = getProcessorByIndex (static_cast<ProcessorIndex> (i));
        if (auto* rdProc = dynamic_cast<RD_Processor*> (audioProc))
            removeChild (rdProc);
    }
}

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

    mGainNodeID         = mGraph.addNode (std::make_unique<GainProcessor>())->nodeID;
    mGrainShifterNodeID = mGraph.addNode (std::make_unique<GrainShifterProcessor>())->nodeID;
    mOscillatorNodeID   = mGraph.addNode (std::make_unique<OscillatorProcessor>())->nodeID;
    mSynthNodeID        = mGraph.addNode (std::make_unique<SynthProcessor>())->nodeID;
    mPulsarNodeID       = mGraph.addNode (std::make_unique<PulsarProcessor>())->nodeID;

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
    else if (index == ProcessorIndex::kOscillator)
    {
        for (auto* node : mGraph.getNodes())
            if (auto* p = dynamic_cast<OscillatorProcessor*> (node->getProcessor()))
                return p;
    }
    else if (index == ProcessorIndex::kSynth)
    {
        for (auto* node : mGraph.getNodes())
            if (auto* p = dynamic_cast<SynthProcessor*> (node->getProcessor()))
                return p;
    }
    else if (index == ProcessorIndex::kPulsar)
    {
        for (auto* node : mGraph.getNodes())
            if (auto* p = dynamic_cast<PulsarProcessor*> (node->getProcessor()))
                return p;
    }
    return nullptr;
}

juce::AudioProcessor* RD_ProcessorSwapper::getActiveProcessor()
{
    return getProcessorByIndex (mActiveProcessorIndex);
}

//==============================================================================
void RD_ProcessorSwapper::doPrepareToPlay (double sampleRate, int samplesPerBlock)
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
        mGraph.removeConnection ({{ mAudioInputNodeID,    ch }, { mGainNodeID,         ch }});
        mGraph.removeConnection ({{ mAudioInputNodeID,    ch }, { mGrainShifterNodeID, ch }});
        mGraph.removeConnection ({{ mAudioInputNodeID,    ch }, { mOscillatorNodeID,   ch }});
        mGraph.removeConnection ({{ mAudioInputNodeID,    ch }, { mSynthNodeID,        ch }});
        mGraph.removeConnection ({{ mAudioInputNodeID,    ch }, { mPulsarNodeID,       ch }});
        mGraph.removeConnection ({{ mGainNodeID,          ch }, { mAudioOutputNodeID,  ch }});
        mGraph.removeConnection ({{ mGrainShifterNodeID,  ch }, { mAudioOutputNodeID,  ch }});
        mGraph.removeConnection ({{ mOscillatorNodeID,    ch }, { mAudioOutputNodeID,  ch }});
        mGraph.removeConnection ({{ mSynthNodeID,         ch }, { mAudioOutputNodeID,  ch }});
        mGraph.removeConnection ({{ mPulsarNodeID,        ch }, { mAudioOutputNodeID,  ch }});
    }

    juce::AudioProcessorGraph::NodeID activeNodeID;
    switch (mActiveProcessorIndex)
    {
        case ProcessorIndex::kGain:         activeNodeID = mGainNodeID;         break;
        case ProcessorIndex::kGrainShifter: activeNodeID = mGrainShifterNodeID; break;
        case ProcessorIndex::kOscillator:   activeNodeID = mOscillatorNodeID;   break;
        case ProcessorIndex::kSynth:        activeNodeID = mSynthNodeID;        break;
        case ProcessorIndex::kPulsar:       activeNodeID = mPulsarNodeID;       break;
        default:                            activeNodeID = mGainNodeID;         break;
    }

    // Bypass every non-active node so AudioProcessorGraph's buffer-aliasing
    // can't let a disconnected processor's output leak into the output bus.
    for (auto* node : mGraph.getNodes())
        if (node != nullptr)
            node->setBypassed (node->nodeID != activeNodeID
                            && node->nodeID != mAudioInputNodeID
                            && node->nodeID != mAudioOutputNodeID);

    for (int ch = 0; ch < 2; ++ch)
    {
        mGraph.addConnection ({{ mAudioInputNodeID, ch }, { activeNodeID,        ch }});
        mGraph.addConnection ({{ activeNodeID,      ch }, { mAudioOutputNodeID,  ch }});
    }
}
