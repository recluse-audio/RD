#include "OscillatorProcessor.h"
#include "OSCILLATOR/Oscillator.h"
#include "WAVEFORM/Wavetable.h"

OscillatorProcessor::OscillatorProcessor()
: RD_Processor(),
  mWavetable  (std::make_unique<rd_dsp::Wavetable>()),
  mOscillator (std::make_unique<rd_dsp::Oscillator>(*mWavetable)),
  mAPVTS (*this, nullptr, "Parameters", createParameterLayout())
{
    // Seed wavetable so oscillator has a waveform to read. Defaults to sine
    // (first shape in fillWithBasicShapes) at normalized wave position 0.
    mWavetable->fillWithBasicShapes (2048);

    mAPVTS.addParameterListener (OscillatorParams::kOnOffID,     this);
    mAPVTS.addParameterListener (OscillatorParams::kFrequencyID, this);
    mAPVTS.addParameterListener (OscillatorParams::kGainID,      this);
}

OscillatorProcessor::~OscillatorProcessor()
{
    mAPVTS.removeParameterListener (OscillatorParams::kOnOffID,     this);
    mAPVTS.removeParameterListener (OscillatorParams::kFrequencyID, this);
    mAPVTS.removeParameterListener (OscillatorParams::kGainID,      this);
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
    if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (
                    mAPVTS.getParameter (OscillatorParams::kGainID)))
    {
        parameterChanged (OscillatorParams::kGainID, p->get());
    }
}

void OscillatorProcessor::doProcessBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    auto* readPtr  = buffer.getArrayOfReadPointers();
    auto* writePtr = buffer.getArrayOfWritePointers();
    mOscillator->process(readPtr, writePtr, buffer.getNumChannels(), buffer.getNumSamples());

    buffer.applyGain (mGain.get());
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
    else if (parameterID == OscillatorParams::kGainID)
    {
        mGain.set (newValue);
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

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        OscillatorParams::kGainID,
        "Oscillator Gain",
        juce::NormalisableRange<float> (0.f, 1.f, 0.0001f),
        0.3f));

    return { params.begin(), params.end() };
}
