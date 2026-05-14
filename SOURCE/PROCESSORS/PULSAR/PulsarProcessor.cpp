#include "PulsarProcessor.h"
#include "PULSAR/PulsarTrain.h"
#include "EDITORS/PulsarEditor.h"

PulsarProcessor::PulsarProcessor()
: RD_Processor(),
  mPulsarTrain (std::make_unique<rd_dsp::PulsarTrain>()),
  mAPVTS (*this, nullptr, "Parameters", createParameterLayout())
{
    mAPVTS.addParameterListener (PulsarParams::kFundamentalFreqID, this);
    mAPVTS.addParameterListener (PulsarParams::kFormantFreqID,     this);
    mAPVTS.addParameterListener (PulsarParams::kWavePositionID,    this);
    mAPVTS.addParameterListener (PulsarParams::kGainID,            this);
    mAPVTS.addParameterListener (PulsarParams::kOnOffID,           this);
}

PulsarProcessor::~PulsarProcessor()
{
    mAPVTS.removeParameterListener (PulsarParams::kFundamentalFreqID, this);
    mAPVTS.removeParameterListener (PulsarParams::kFormantFreqID,     this);
    mAPVTS.removeParameterListener (PulsarParams::kWavePositionID,    this);
    mAPVTS.removeParameterListener (PulsarParams::kGainID,            this);
    mAPVTS.removeParameterListener (PulsarParams::kOnOffID,           this);
}

void PulsarProcessor::doPrepareToPlay (double sampleRate, int maxBlockSize)
{
    mPulsarTrain->prepare (sampleRate, maxBlockSize);

    auto pushFloat = [this] (const juce::String& id)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (mAPVTS.getParameter (id)))
            parameterChanged (id, p->get());
    };

    pushFloat (PulsarParams::kFundamentalFreqID);
    pushFloat (PulsarParams::kFormantFreqID);
    pushFloat (PulsarParams::kWavePositionID);
    pushFloat (PulsarParams::kGainID);

    if (auto* onOff = dynamic_cast<juce::AudioParameterBool*> (mAPVTS.getParameter (PulsarParams::kOnOffID)))
        parameterChanged (PulsarParams::kOnOffID, onOff->get() ? 1.f : 0.f);
}

void PulsarProcessor::doProcessBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    auto* readPtr  = buffer.getArrayOfReadPointers();
    auto* writePtr = buffer.getArrayOfWritePointers();
    mPulsarTrain->process (readPtr, writePtr, buffer.getNumChannels(), buffer.getNumSamples());

    buffer.applyGain (mGain.get());
}

void PulsarProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == PulsarParams::kFundamentalFreqID)
        mPulsarTrain->setEmissionRate (newValue);
    else if (parameterID == PulsarParams::kFormantFreqID)
        mPulsarTrain->setFormantFreq (newValue);
    else if (parameterID == PulsarParams::kWavePositionID)
        mPulsarTrain->setWavePosition (newValue);
    else if (parameterID == PulsarParams::kGainID)
        mGain.set (newValue);
    else if (parameterID == PulsarParams::kOnOffID)
    {
        if (newValue >= 0.5f)
            mPulsarTrain->start();
        else
            mPulsarTrain->stop();
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout PulsarProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        PulsarParams::kFundamentalFreqID, "Fundamental Frequency",
        juce::NormalisableRange<float> (rd_dsp::PulsarTrain::kMinEmissionRate,
                                        rd_dsp::PulsarTrain::kMaxEmissionRate,
                                        0.01f),
        10.f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        PulsarParams::kFormantFreqID, "Formant Frequency",
        juce::NormalisableRange<float> (rd_dsp::PulsarTrain::kMinFormantFreq,
                                        rd_dsp::PulsarTrain::kMaxFormantFreq,
                                        0.01f),
        440.f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        PulsarParams::kWavePositionID, "Wave Position",
        juce::NormalisableRange<float> (0.f, 1.f, 0.0001f),
        0.f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        PulsarParams::kGainID, "Gain",
        juce::NormalisableRange<float> (0.f, 1.f, 0.0001f),
        0.3f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        PulsarParams::kOnOffID, "Pulsar On/Off", false));

    return { params.begin(), params.end() };
}

juce::AudioProcessorEditor* PulsarProcessor::createEditor()
{
    return new PulsarEditor (*this);
}
