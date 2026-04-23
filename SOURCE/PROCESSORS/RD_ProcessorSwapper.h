#pragma once
#include "Util/Juce_Header.h"
#include "GAIN/GainProcessor.h"
#include "GRAIN/GrainShifterProcessor.h"
#include "FX/Fade.h"

class RD_ProcessorSwapper : public juce::AudioProcessor
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
