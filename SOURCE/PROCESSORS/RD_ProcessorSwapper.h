#pragma once
#include "Util/Juce_Header.h"
#include "BASE/RD_Processor.h"
#include "GAIN/GainProcessor.h"
#include "GRAIN/GrainShifterProcessor.h"
#include "FX/Fade.h"

class RD_ProcessorSwapper : public RD_Processor
{
public:
    enum class ProcessorIndex
    {
        kGain    = 0,
        kGrainShifter = 1,
        kCount
    };

    RD_ProcessorSwapper();
    ~RD_ProcessorSwapper() override;

    void doPrepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void doProcessBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override             { return "RD Processor Swapper"; }

    //==============================================================================
    juce::AudioProcessor* getActiveProcessor();
    juce::AudioProcessor* getProcessorByIndex (ProcessorIndex index);
    int getNumProcessors() const { return static_cast<int> (ProcessorIndex::kCount); }
    void setActiveProcessor (ProcessorIndex index);
    const ProcessorIndex& getActiveProcessorIndex() const { return mActiveProcessorIndex; }
    Fade::FadeState getFadeState() const { return mFade.getCurrentState(); }


private:
    juce::AudioProcessorGraph mGraph;

    juce::AudioProcessorGraph::NodeID mAudioInputNodeID;
    juce::AudioProcessorGraph::NodeID mAudioOutputNodeID;
    juce::AudioProcessorGraph::NodeID mGainNodeID;
    juce::AudioProcessorGraph::NodeID mGrainShifterNodeID;

    ProcessorIndex mActiveProcessorIndex { ProcessorIndex::kGain };

    Fade mFade;
    void _buildGraph();
    void _applyProcessorSwap();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RD_ProcessorSwapper)
};
