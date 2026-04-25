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
    mSampleRate = sampleRate;
    mBlockSize  = samplesPerBlock;
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

void RD_Processor::processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
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

juce::AudioProcessorValueTreeState& RD_Processor::getAPVTS()
{
    return mBaseAPVTS;
}

juce::File RD_Processor::createProcessorDataLogFile()
{
    auto timestamp  = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    auto sessionDir = getOutputFile().getChildFile (timestamp);
    sessionDir.createDirectory();

    auto xmlState   = getAPVTS().copyState().createXml();
    juce::String apvtsXml = xmlState != nullptr ? xmlState->toString() : "";

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("processorName", getName());
    obj->setProperty ("apvts",         apvtsXml);

    auto logFile = sessionDir.getChildFile ("processor_state.json");
    logFile.replaceWithText (juce::JSON::toString (juce::var (obj.get())));

    return logFile;
}

juce::File RD_Processor::createProcessBlockDataLogFile (juce::AudioBuffer<float> processBuffer, bool isPreProcessing)
{
    auto now        = juce::Time::getCurrentTime();
    auto timestamp  = now.formatted ("%Y-%m-%d_%H-%M-%S")
                    + "_" + juce::String (now.getMilliseconds()).paddedLeft ('0', 3);
    auto sessionDir = getOutputFile().getChildFile (timestamp);
    sessionDir.createDirectory();

    juce::String fileName = (isPreProcessing ? "preprocess_" : "postprocess_")
                          + juce::String (processBuffer.getNumChannels()) + "ch_"
                          + juce::String (processBuffer.getNumSamples())  + "smp.csv";

    auto logFile = sessionDir.getChildFile (fileName);

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

    for (int s = 0; s < numSamples; ++s)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (ch > 0) csv << ",";
            csv << juce::String (processBuffer.getSample (ch, s), 8);
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
