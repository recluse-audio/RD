#include "PulsarProcessor.h"
#include "PULSAR/PulsarSynth.h"
#include "EDITORS/PulsarEditor.h"

PulsarProcessor::PulsarProcessor()
: RD_Processor(),
  mPulsar (std::make_unique<rd_dsp::PulsarSynth>()),
  mAPVTS (*this, nullptr, "Parameters", createParameterLayout())
{
    mPulsar->setNumVoices (PulsarParams::kDefaultNumVoices);

    mAPVTS.addParameterListener (PulsarParams::kRateID,           this);
    mAPVTS.addParameterListener (PulsarParams::kGrainFrequencyID, this);
    mAPVTS.addParameterListener (PulsarParams::kGrainCyclesID,    this);
    mAPVTS.addParameterListener (PulsarParams::kWavePositionID,   this);
    mAPVTS.addParameterListener (PulsarParams::kGainID,           this);
}

PulsarProcessor::~PulsarProcessor()
{
    mAPVTS.removeParameterListener (PulsarParams::kRateID,           this);
    mAPVTS.removeParameterListener (PulsarParams::kGrainFrequencyID, this);
    mAPVTS.removeParameterListener (PulsarParams::kGrainCyclesID,    this);
    mAPVTS.removeParameterListener (PulsarParams::kWavePositionID,   this);
    mAPVTS.removeParameterListener (PulsarParams::kGainID,           this);
}

void PulsarProcessor::doPrepareToPlay (double sampleRate, int maxBlockSize)
{
    mPulsar->prepare (sampleRate, maxBlockSize);

    auto pushFloat = [this] (const juce::String& id)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (mAPVTS.getParameter (id)))
            parameterChanged (id, p->get());
    };
    auto pushInt = [this] (const juce::String& id)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterInt*> (mAPVTS.getParameter (id)))
            parameterChanged (id, static_cast<float> (p->get()));
    };

    pushFloat (PulsarParams::kRateID);
    pushFloat (PulsarParams::kGrainFrequencyID);
    pushInt   (PulsarParams::kGrainCyclesID);
    pushFloat (PulsarParams::kWavePositionID);
    pushFloat (PulsarParams::kGainID);
}

void PulsarProcessor::doProcessBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    // Pulsar additively renders grains; clear first.
    buffer.clear();

    auto* readPtr  = buffer.getArrayOfReadPointers();
    auto* writePtr = buffer.getArrayOfWritePointers();
    mPulsar->process (readPtr, writePtr, buffer.getNumChannels(), buffer.getNumSamples());

    buffer.applyGain (mGain.get());
}

void PulsarProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == PulsarParams::kRateID)
        mPulsar->setRate (newValue);
    else if (parameterID == PulsarParams::kGrainFrequencyID)
        mPulsar->setGrainFrequency (newValue);
    else if (parameterID == PulsarParams::kGrainCyclesID)
        mPulsar->setGrainCycles (static_cast<int> (newValue));
    else if (parameterID == PulsarParams::kWavePositionID)
        mPulsar->setWavePosition (newValue);
    else if (parameterID == PulsarParams::kGainID)
        mGain.set (newValue);
}

juce::AudioProcessorValueTreeState::ParameterLayout PulsarProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        PulsarParams::kRateID,
        "Rate",
        juce::NormalisableRange<float> (0.f, 200.f, 0.01f),
        10.f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        PulsarParams::kGrainFrequencyID,
        "Grain Frequency",
        juce::NormalisableRange<float> (20.f, 8000.f, 0.01f, 0.3f),
        220.f));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        PulsarParams::kGrainCyclesID,
        "Grain Cycles",
        1, 32, 2));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        PulsarParams::kWavePositionID,
        "Wave Position",
        juce::NormalisableRange<float> (0.f, 1.f, 0.0001f),
        0.f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        PulsarParams::kGainID,
        "Gain",
        juce::NormalisableRange<float> (0.f, 1.f, 0.0001f),
        0.3f));

    return { params.begin(), params.end() };
}

juce::AudioProcessorEditor* PulsarProcessor::createEditor()
{
    return new PulsarEditor (*this);
}
