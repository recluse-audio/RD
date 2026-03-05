#pragma once
#include "Util/Juce_Header.h"
#include "GAIN/GainProcessor.h"
#include "TDPSOLA/TDPSOLA_Processor.h"

class RD_ProcessorSwapper : public juce::AudioProcessor
{
public:
    enum class ProcessorIndex
    {
        kGain    = 0,
        kTDPSOLA = 1
    };

    RD_ProcessorSwapper();
    ~RD_ProcessorSwapper() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override             { return "RD"; }
    bool acceptsMidi() const override                       { return false; }
    bool producesMidi() const override                      { return false; }
    double getTailLengthSeconds() const override            { return 0.0; }
    int getNumPrograms() override                           { return 1; }
    int getCurrentProgram() override                        { return 0; }
    void setCurrentProgram (int) override                   {}
    const juce::String getProgramName (int) override        { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& d) override    { juce::ignoreUnused (d); }
    void setStateInformation (const void* d, int n) override    { juce::ignoreUnused (d, n); }

    //==============================================================================
    juce::AudioProcessor* getActiveProcessor();
    juce::AudioProcessor* getProcessorByIndex (ProcessorIndex index);
    void setActiveProcessor (ProcessorIndex index) { mActiveProcessor = index; }
    const ProcessorIndex& getActiveProcessorIndex() const { return mActiveProcessor; }

private:
    juce::AudioProcessorGraph mGraph;

    juce::AudioProcessorGraph::NodeID mAudioInputNodeID;
    juce::AudioProcessorGraph::NodeID mAudioOutputNodeID;
    juce::AudioProcessorGraph::NodeID mGainNodeID;
    juce::AudioProcessorGraph::NodeID mTDPSOLANodeID;

    ProcessorIndex mActiveProcessor { ProcessorIndex::kGain };

    void _buildGraph();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RD_ProcessorSwapper)
};
