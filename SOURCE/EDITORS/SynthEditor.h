#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/SYNTH/SynthProcessor.h"

class SynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit SynthEditor (SynthProcessor& processor);
    ~SynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    int  _getMidiNoteFromText() const;
    void _sendNoteOn();
    void _sendNoteOff();

    SynthProcessor& mProcessor;

    juce::Label     mNoteLabel;
    juce::TextEditor mNoteEntry;
    juce::TextButton mNoteOnButton  { "Note On"  };
    juce::TextButton mNoteOffButton { "Note Off" };

    juce::Slider mWavePositionSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
    juce::Label  mWavePositionLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mWavePositionAttachment;

    juce::Slider mGainSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
    juce::Label  mGainLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEditor)
};
