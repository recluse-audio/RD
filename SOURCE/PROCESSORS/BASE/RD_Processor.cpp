#include "RD_Processor.h"

RD_Processor::RD_Processor()
: AudioProcessor (_getDefaultBusesProperties())
, mBaseAPVTS (*this, nullptr, "Parameters", _createParameterLayout())
{
    mBaseAPVTS.addParameterListener ("gain", this);
    mGainValue.set (*mBaseAPVTS.getRawParameterValue ("gain"));
}

RD_Processor::~RD_Processor()
{
    mBaseAPVTS.removeParameterListener ("gain", this);
}

void RD_Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate         = sampleRate;
    mBlockSize          = samplesPerBlock;
    mProcessSampleCount = 0;
}

void RD_Processor::releaseResources()
{
}

bool RD_Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    juce::ignoreUnused (layouts);
    return true;
}

const juce::String RD_Processor::getName() const
{
    return "RD_Processor";
}

void RD_Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiBuffer)
{
    if(this->getIsLogging())
        this->createProcessBlockDataLogFile(buffer, true);

    // This is where child class processors will do their magic
    doProcessBlock (buffer, midiBuffer);

    if(this->getIsLogging())
        this->createProcessBlockDataLogFile(buffer, false);
}

void RD_Processor::doProcessBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiBuffer)
{
    juce::ignoreUnused (buffer, midiBuffer);
}

juce::AudioProcessorEditor* RD_Processor::createEditor()
{
    return nullptr;
}

bool RD_Processor::hasEditor() const
{
    return false;
}

bool RD_Processor::acceptsMidi() const
{
    return false;
}

bool RD_Processor::producesMidi() const
{
    return false;
}

double RD_Processor::getTailLengthSeconds() const
{
    return 0.0;
}

int RD_Processor::getNumPrograms()
{
    return 1;
}

int RD_Processor::getCurrentProgram()
{
    return 0;
}

void RD_Processor::setCurrentProgram (int)
{
}

const juce::String RD_Processor::getProgramName (int)
{
    return "None";
}

void RD_Processor::changeProgramName (int, const juce::String&)
{
}

void RD_Processor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void RD_Processor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

const double RD_Processor::getLastSampleRateFromPrepareToPlay() const
{
    return mSampleRate;
}

const int RD_Processor::getLastBlockSizeFromPrepareToPlay() const
{
    return mBlockSize;
}

juce::int64 RD_Processor::getProcessSampleCount() const
{
    return mProcessSampleCount;
}

juce::AudioProcessorValueTreeState& RD_Processor::getAPVTS()
{
    return mBaseAPVTS;
}

juce::File RD_Processor::createProcessorDataLogFile()
{
    createOutputDirectory (getOutputFile());

    auto xmlState   = getAPVTS().copyState().createXml();
    juce::String apvtsXml = xmlState != nullptr ? xmlState->toString() : "";

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("processorName", getName());
    obj->setProperty ("apvts",         apvtsXml);

    auto logFile = getOutputFile().getChildFile ("processor_state.json");
    logFile.replaceWithText (juce::JSON::toString (juce::var (obj.get())));

    return logFile;
}


juce::File RD_Processor::createProcessBlockDataLogFile (juce::AudioBuffer<float> processBuffer, bool isPreProcessing)
{
    createOutputDirectory (getOutputFile());

    juce::String fileName = (isPreProcessing ? "preprocess_" : "postprocess_")
                          + juce::String (processBuffer.getNumChannels()) + "ch_"
                          + juce::String (processBuffer.getNumSamples())  + "smp.csv";

    auto logFile = getOutputFile().getChildFile (fileName);

    const int numChannels = processBuffer.getNumChannels();
    const int numSamples  = processBuffer.getNumSamples();

    juce::String csv;
    csv.preallocateBytes (static_cast<size_t> (numSamples * numChannels * 12));

    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (ch > 0) csv << ",";
        csv << "ch" << ch;
    }
    csv << "\n";

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (ch > 0) csv << ",";
            csv << juce::String (processBuffer.getSample (ch, sampleIndex), 8);
        }
        csv << "\n";
    }

    logFile.replaceWithText (csv);
    return logFile;
}

void RD_Processor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "gain")
        _updateGainValue (newValue);
}

void RD_Processor::setGain (float newGain)
{
    mGainValue.set (newGain);
}

//==================================
// PRIVATE
//==================================

juce::AudioProcessor::BusesProperties RD_Processor::_getDefaultBusesProperties()
{
    return BusesProperties()
                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                .withOutput ("Output", juce::AudioChannelSet::stereo(), true);
}

juce::AudioProcessorValueTreeState::ParameterLayout RD_Processor::_createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "gain",
        "Gain",
        0.0f,
        1.0f,
        1.0f));

    return { params.begin(), params.end() };
}

void RD_Processor::_updateGainValue (float newValue)
{
    mGainValue.set (newValue);
}
