#pragma once
#include "Util/Juce_Header.h"
#include "GAIN/GainProcessor.h"

class RD_PluginProcessor : public juce::AudioProcessor
{
public:
    RD_PluginProcessor();
    ~RD_PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    GainProcessor* getGainProcessor()
    {
        if (mGainNode != nullptr)
            return dynamic_cast<GainProcessor*> (mGainNode->getProcessor());
        return nullptr;
    }

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

private:
    juce::AudioProcessorGraph mGraph;

    juce::AudioProcessorGraph::Node::Ptr mAudioInputNode;
    juce::AudioProcessorGraph::Node::Ptr mAudioOutputNode;
    juce::AudioProcessorGraph::Node::Ptr mGainNode;

    void _buildGraph();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RD_PluginProcessor)
};
