#include "SynthEditor.h"

SynthEditor::SynthEditor (SynthProcessor& processor)
    : AudioProcessorEditor (processor)
    , mProcessor (processor)
    , mWavePositionAttachment (processor.getAPVTS(), SynthParams::kWavePositionID, mWavePositionSlider)
    , mGainAttachment         (processor.getAPVTS(), SynthParams::kGainID,         mGainSlider)
{
    setName ("SynthEditor_Component");

    mNoteLabel.setText ("MIDI Note (0-127)", juce::dontSendNotification);
    mNoteLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (mNoteLabel);

    mNoteEntry.setInputRestrictions (3, "0123456789");
    mNoteEntry.setText ("60", juce::dontSendNotification);
    mNoteEntry.setJustification (juce::Justification::centred);
    addAndMakeVisible (mNoteEntry);

    mNoteOnButton.onClick  = [this] { _sendNoteOn();  };
    mNoteOffButton.onClick = [this] { _sendNoteOff(); };
    addAndMakeVisible (mNoteOnButton);
    addAndMakeVisible (mNoteOffButton);

    mWavePositionLabel.setText ("Wave Position", juce::dontSendNotification);
    mWavePositionLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (mWavePositionLabel);
    addAndMakeVisible (mWavePositionSlider);

    mGainLabel.setText ("Gain", juce::dontSendNotification);
    mGainLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (mGainLabel);
    addAndMakeVisible (mGainSlider);

    setSize (320, 460);
}

SynthEditor::~SynthEditor() {}

int SynthEditor::_getMidiNoteFromText() const
{
    const int n = mNoteEntry.getText().getIntValue();
    return juce::jlimit (0, 127, n);
}

void SynthEditor::_sendNoteOn()
{
    const int note = _getMidiNoteFromText();
    auto msg = juce::MidiMessage::noteOn (1, note, (juce::uint8) 100);
    msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    mProcessor.getMidiCollector().addMessageToQueue (msg);
}

void SynthEditor::_sendNoteOff()
{
    const int note = _getMidiNoteFromText();
    auto msg = juce::MidiMessage::noteOff (1, note, (juce::uint8) 0);
    msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    mProcessor.getMidiCollector().addMessageToQueue (msg);
}

void SynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void SynthEditor::resized()
{
    auto area = getLocalBounds().reduced (10);

    mNoteLabel.setBounds (area.removeFromTop (20));
    mNoteEntry.setBounds (area.removeFromTop (28));
    area.removeFromTop (6);

    auto buttonRow = area.removeFromTop (32);
    const int half = buttonRow.getWidth() / 2;
    mNoteOnButton .setBounds (buttonRow.removeFromLeft (half).reduced (2, 0));
    mNoteOffButton.setBounds (buttonRow.reduced (2, 0));

    area.removeFromTop (12);
    mWavePositionLabel .setBounds (area.removeFromTop (20));
    mWavePositionSlider.setBounds (area.removeFromTop (60));

    area.removeFromTop (12);
    mGainLabel .setBounds (area.removeFromTop (20));
    mGainSlider.setBounds (area.removeFromTop (60));
}
