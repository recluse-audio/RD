#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/RD_PluginProcessor.h"
#include "ProcessorControlsView.h"

class RD_PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit RD_PluginEditor (RD_PluginProcessor& p);
    ~RD_PluginEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    RD_PluginProcessor& processorRef;
    std::unique_ptr<ProcessorControlsView> mControlsView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RD_PluginEditor)
};
