#include "RD_ProcessorSwapperEditor.h"

RD_ProcessorSwapperEditor::RD_ProcessorSwapperEditor (RD_ProcessorSwapper& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    mProcessorSelector.setSelectedId (static_cast<int> (processorRef.getActiveProcessorIndex()) + 1, juce::dontSendNotification);
    mProcessorSelector.onChange = [this] { _onProcessorSelected (mProcessorSelector.getSelectedId()); };
    addAndMakeVisible (mProcessorSelector);

    _addChildEditors();
    updateActiveEditor();

    setSize (400, 500);
}

//==============================================================================
RD_ProcessorSwapperEditor::~RD_ProcessorSwapperEditor() {}


//==============================================================================
void RD_ProcessorSwapperEditor::updateActiveEditor()
{
    const int activeIndex = static_cast<int> (processorRef.getActiveProcessorIndex());

    for (int i = 0; i < static_cast<int> (mChildEditors.size()); ++i)
        if (auto* editor = mChildEditors[i].get())
            editor->setVisible (i == activeIndex);

    resized();
}

//==============================================================================
void RD_ProcessorSwapperEditor::_addChildEditors()
{
    for (int i = 0; i < processorRef.getNumProcessors(); ++i)
    {
        auto index = static_cast<RD_ProcessorSwapper::ProcessorIndex> (i);
        auto* processor = processorRef.getProcessorByIndex (index);
        if (processor == nullptr) continue;

        mProcessorSelector.addItem (processor->getName(), i + 1);

        auto* editor = processor->createEditor();
        if (editor == nullptr)
        {
            // Preserve index alignment with ProcessorIndex — null slot for
            // processors that don't expose an editor.
            mChildEditors.emplace_back (nullptr);
            continue;
        }

        addChildComponent (*editor);
        mChildEditors.emplace_back (editor);
    }


}


//==============================================================================
void RD_ProcessorSwapperEditor::_onProcessorSelected (int comboBoxId)
{
    auto index = static_cast<RD_ProcessorSwapper::ProcessorIndex> (comboBoxId - 1);
    processorRef.setActiveProcessor (index);
    updateActiveEditor();
}

//==============================================================================
void RD_ProcessorSwapperEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

//==============================================================================
void RD_ProcessorSwapperEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    mProcessorSelector.setBounds (area.removeFromTop (24));
    area.removeFromTop (8);

    for (auto& editor : mChildEditors)
        if (editor != nullptr)
            editor->setBounds (area);
}
