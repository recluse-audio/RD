#include "GainProcessor.h"
#include "EDITORS/GainEditor.h"

GainProcessor::GainProcessor()
: RD_Processor()
, apvts(*this, nullptr, "Parameters", _createParameterLayout())
{
    apvts.addParameterListener ("gain", this);
    mGainValue.set (*apvts.getRawParameterValue ("gain"));
}

GainProcessor::~GainProcessor()
{
    apvts.removeParameterListener ("gain", this);
}

void GainProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    RD_Processor::prepareToPlay (sampleRate, samplesPerBlock);
}

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

//===================
//
juce::AudioProcessorEditor* GainProcessor::createEditor()
{
    return new GainEditor (*this);
}

juce::AudioProcessorValueTreeState& GainProcessor::getAPVTS()
{
    return apvts;
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
        0.01f));         // Default value

    return { params.begin(), params.end() };
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
