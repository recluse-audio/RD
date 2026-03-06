#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/RD_ProcessorSwapper.h"

class RD_ProcessorSwapperEditor : public juce::AudioProcessorEditor
{
public:
    explicit RD_ProcessorSwapperEditor (RD_ProcessorSwapper& p);
    ~RD_ProcessorSwapperEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void updateActiveEditor();

private:
    void _onProcessorSelected (int comboBoxId);
    void _addChildEditors();

    RD_ProcessorSwapper& processorRef;
    juce::ComboBox mProcessorSelector;
    std::vector<std::unique_ptr<juce::AudioProcessorEditor>> mChildEditors;

    friend class RD_ProcessorSwapperEditorTests;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RD_ProcessorSwapperEditor)
};
