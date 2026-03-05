#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/RD_ProcessorSwapper.h"
#include "COMPONENTS/ActiveProcessorView.h"

class RD_PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit RD_PluginEditor (RD_ProcessorSwapper& p);
    ~RD_PluginEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    RD_ProcessorSwapper& processorRef;
    std::unique_ptr<ActiveProcessorView> mControlsView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RD_PluginEditor)
};
