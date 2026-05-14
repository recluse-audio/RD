#include "SynthProcessor.h"
#include "SYNTH/Synth.h"
#include "EDITORS/SynthEditor.h"

SynthProcessor::SynthProcessor()
: RD_Processor(),
  mSynth (std::make_unique<rd_dsp::Synth>()),
  mAPVTS (*this, nullptr, "Parameters", createParameterLayout())
{
    mSynth->setNumVoices (SynthParams::kDefaultNumVoices);
    mAPVTS.addParameterListener (SynthParams::kWavePositionID, this);
    mAPVTS.addParameterListener (SynthParams::kGainID,         this);
}

SynthProcessor::~SynthProcessor()
{
    mAPVTS.removeParameterListener (SynthParams::kWavePositionID, this);
    mAPVTS.removeParameterListener (SynthParams::kGainID,         this);
}

void SynthProcessor::doPrepareToPlay (double sampleRate, int maxBlockSize)
{
    mSynth->prepare (sampleRate, maxBlockSize);

    // MidiMessageCollector needs sample rate for timestamping enqueued events.
    mMidiCollector.reset (sampleRate);

    if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (
                    mAPVTS.getParameter (SynthParams::kWavePositionID)))
    {
        parameterChanged (SynthParams::kWavePositionID, p->get());
    }
    if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (
                    mAPVTS.getParameter (SynthParams::kGainID)))
    {
        parameterChanged (SynthParams::kGainID, p->get());
    }
}

void SynthProcessor::doProcessBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Drain UI-thread MIDI into the per-block buffer (lock-free, real-time safe).
    mMidiCollector.removeNextBlockOfMessages (midiMessages, buffer.getNumSamples());

    for (const auto meta : midiMessages)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
            mSynth->noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            mSynth->noteOff (msg.getNoteNumber(), msg.getFloatVelocity());
    }

    // Synth additively renders active voices on top of incoming buffer; clear first.
    buffer.clear();

    auto* readPtr  = buffer.getArrayOfReadPointers();
    auto* writePtr = buffer.getArrayOfWritePointers();
    mSynth->process (readPtr, writePtr, buffer.getNumChannels(), buffer.getNumSamples());

    buffer.applyGain (mGain.get());
}

void SynthProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == SynthParams::kWavePositionID)
        mSynth->setWavePosition (newValue);
    else if (parameterID == SynthParams::kGainID)
        mGain.set (newValue);
}

juce::AudioProcessorValueTreeState::ParameterLayout SynthProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        SynthParams::kWavePositionID,
        "Wave Position",
        juce::NormalisableRange<float> (0.f, 1.f, 0.0001f),
        0.f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        SynthParams::kGainID,
        "Gain",
        juce::NormalisableRange<float> (0.f, 1.f, 0.0001f),
        0.3f));

    return { params.begin(), params.end() };
}

juce::AudioProcessorEditor* SynthProcessor::createEditor()
{
    return new SynthEditor (*this);
}
