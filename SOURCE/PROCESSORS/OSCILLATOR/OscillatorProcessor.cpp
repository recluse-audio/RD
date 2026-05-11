#include "OscillatorProcessor.h"
#include "OSCILLATOR/Oscillator.h"

OscillatorProcessor::OscillatorProcessor()
: RD_Processor(),
  mOscillator (std::make_unique<rd_dsp::Oscillator>()),
  mAPVTS (*this, nullptr, "Parameters", createParameterLayout())
{
    mAPVTS.addParameterListener (OscillatorParams::kOnOffID,     this);
    mAPVTS.addParameterListener (OscillatorParams::kFrequencyID, this);
}

OscillatorProcessor::~OscillatorProcessor()
{
    mAPVTS.removeParameterListener (OscillatorParams::kOnOffID,     this);
    mAPVTS.removeParameterListener (OscillatorParams::kFrequencyID, this);
}

void OscillatorProcessor::doPrepareToPlay(double sampleRate, int maxBlockSize)
{
    mOscillator->prepare(sampleRate, maxBlockSize);

    // Seed oscillator state from current APVTS values. Listener only fires on
    // change, so default values would otherwise never reach the oscillator.
    if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (
                    mAPVTS.getParameter (OscillatorParams::kFrequencyID)))
    {
        parameterChanged (OscillatorParams::kFrequencyID, p->get());
    }
    if (auto* p = dynamic_cast<juce::AudioParameterBool*> (
                    mAPVTS.getParameter (OscillatorParams::kOnOffID)))
    {
        parameterChanged (OscillatorParams::kOnOffID, p->get() ? 1.f : 0.f);
    }
}

void OscillatorProcessor::doProcessBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    auto* readPtr  = buffer.getArrayOfReadPointers();
    auto* writePtr = buffer.getArrayOfWritePointers();
    mOscillator->process(readPtr, writePtr, buffer.getNumChannels(), buffer.getNumSamples());
}

void OscillatorProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == OscillatorParams::kFrequencyID)
    {
        mOscillator->setFreq (newValue);
    }
    else if (parameterID == OscillatorParams::kOnOffID)
    {
        if (newValue > 0.5f)
            mOscillator->start();
        else
            mOscillator->stop();
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout OscillatorProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        OscillatorParams::kOnOffID,
        "Oscillator On/Off",
        false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        OscillatorParams::kFrequencyID,
        "Oscillator Frequency",
        juce::NormalisableRange<float> (20.f, 20000.f, 0.01f, 0.3f),
        440.f));

    return { params.begin(), params.end() };
}
