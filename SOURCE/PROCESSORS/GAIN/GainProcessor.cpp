#include "GainProcessor.h"

GainProcessor::GainProcessor()
: AudioProcessor (_getBusesProperties())
, apvts(*this, nullptr, "Parameters", _createParameterLayout())
{

}

GainProcessor::~GainProcessor() {}

void GainProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) 
{
    mSampleRate = sampleRate;
    mBlockSize = samplesPerBlock;
}

void GainProcessor::releaseResources() {}

void GainProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto gainValue = mGainValue.get(); // Get the current gain value

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            auto inputSample = buffer.getSample(ch, sampleIndex);
            auto outputSample = inputSample * gainValue;
            buffer.setSample(ch, sampleIndex, outputSample);
        }
    }
}

//===================
//
void GainProcessor::parameterChanged(const juce::String& parameterID, float newValue) 
{
    if(parameterID == "gain")
    {
        this->_updateGainValue(newValue);
    }
}


//==================================
// PRIVATE
//==================================

//===================
//
void GainProcessor::_updateGainValue(float newValue)
{
    mGainValue.set(newValue);
}

//===================
//
juce::AudioProcessorValueTreeState::ParameterLayout GainProcessor::_createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Add a gain parameter as an example
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain",         // Parameter ID
        "Gain",         // Parameter name
        0.0,           // Min value
        1.0f,           // Max value
        1.f));         // Default value

    return { params.begin(), params.end() };
}

//====================
//
juce::AudioProcessor::BusesProperties GainProcessor::_getBusesProperties()
{
    return BusesProperties()
                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                .withOutput("Output", juce::AudioChannelSet::stereo(), true);
}

//====================
//
void GainProcessor::setGain(float newGain)
{
    mGainValue.set(newGain);
    // TODO: setup the apvts param callback, not doing right now for testing sake
    // auto gainParam = apvts.getParameter("gain");
    // gainParam->beginChangeGesture();
    // gainParam->setValueNotifyingHost(newGain);
    // gainParam->endChangeGesture();
}

//====================
//
const int GainProcessor::getLastBlockSizeFromPrepareToPlay()
{
    return mBlockSize;
}

//====================
//
const double GainProcessor::getLastSampleRateFromPrepareToPlay()
{
    return mSampleRate;
}