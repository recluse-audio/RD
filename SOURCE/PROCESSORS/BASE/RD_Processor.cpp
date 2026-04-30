#include "RD_Processor.h"

RD_Processor::RD_Processor()
: AudioProcessor (_getDefaultBusesProperties())
, mBaseAPVTS (*this, nullptr, "Parameters", _createParameterLayout())
{
    mBaseAPVTS.addParameterListener ("gain", this);
    mGainValue.set (*mBaseAPVTS.getRawParameterValue ("gain"));

    _fireLifecycleLog (LifecycleState::kConstructed);
}

RD_Processor::~RD_Processor()
{
    _fireLifecycleLog (LifecycleState::kDestructing);

    mBaseAPVTS.removeParameterListener ("gain", this);
}

void RD_Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate         = sampleRate;
    mBlockSize          = samplesPerBlock;
    mProcessSampleCount = 0;

    _fireLifecycleLog (LifecycleState::kPreparedToPlay);
}

void RD_Processor::releaseResources()
{
    _fireLifecycleLog (LifecycleState::kReleasingResources);
}

bool RD_Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    juce::ignoreUnused (layouts);
    return true;
}


void RD_Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiBuffer)
{
    mLogBlockStartIndex = mProcessSampleCount;

    if (getIsLogging())
    {
        mLogBuffer.makeCopyOf (buffer);
        _fireLifecycleLog (LifecycleState::kProcessBlockStart);
    }

    doProcessBlock (buffer, midiBuffer);

    if (getIsLogging())
    {
        mLogBuffer.makeCopyOf (buffer);
        _fireLifecycleLog (LifecycleState::kProcessBlockEnd);
    }

    mProcessSampleCount += buffer.getNumSamples();
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



void RD_Processor::startLogging()
{
    setIsLogging (true);

    auto dir = getDataLogOutputDirectory();
    for (auto& f : dir.findChildFiles (juce::File::findFiles, false,
                                       "input_samples_ch*.csv"))
        f.deleteFile();
    for (auto& f : dir.findChildFiles (juce::File::findFiles, false,
                                       "output_samples_ch*.csv"))
        f.deleteFile();
}

void RD_Processor::stopLogging()
{
    setIsLogging (false);
}

bool RD_Processor::doLogData()
{
    switch (mLifecycleState)
    {
        case LifecycleState::kConstructed:        return _logConstructed();
        case LifecycleState::kPreparedToPlay:     return _logPrepareToPlay();
        case LifecycleState::kProcessBlockStart:  return _logProcessBlockStart();
        case LifecycleState::kProcessBlockEnd:    return _logProcessBlockEnd();
        case LifecycleState::kReleasingResources: return _logReleasingResources();
        case LifecycleState::kDestructing:        return _logDestructing();
        case LifecycleState::kIdle:               return DataLogger::doLogData();
    }
    return false;
}

void RD_Processor::_fireLifecycleLog (LifecycleState state)
{
    mLifecycleState = state;
    if (getIsLogging())
        logData();
    mLifecycleState = LifecycleState::kIdle;
}

bool RD_Processor::_logConstructed()
{
    return true;
}

bool RD_Processor::_logPrepareToPlay()
{
    return true;
}

bool RD_Processor::_logProcessBlockStart()
{
    _writeBlockSamplesCsv ("input_samples_ch");
    return true;
}

bool RD_Processor::_logProcessBlockEnd()
{
    _writeBlockSamplesCsv ("output_samples_ch");
    return true;
}

bool RD_Processor::_logReleasingResources()
{
    return true;
}

bool RD_Processor::_logDestructing()
{
    return true;
}


juce::File RD_Processor::createProcessorDataLogFile()
{
    auto xmlState = getAPVTS().copyState().createXml();
    if (xmlState == nullptr)
        xmlState = std::make_unique<juce::XmlElement> ("ProcessorState");

    xmlState->setAttribute ("processorName", getName());

    auto logFile = getDataLogOutputDirectory().getChildFile ("processor_state.xml");
    xmlState->writeTo (logFile);

    return logFile;
}

void RD_Processor::_writeBlockSamplesCsv (const juce::String& filenamePrefix)
{
    const auto dir         = getDataLogOutputDirectory();
    const int  numChannels = mLogBuffer.getNumChannels();
    const int  numSamples  = mLogBuffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto file = dir.getChildFile (filenamePrefix + juce::String (ch) + ".csv");

        juce::String indicesRow, valuesRow;
        indicesRow.preallocateBytes (static_cast<size_t> (numSamples * 8));
        valuesRow .preallocateBytes (static_cast<size_t> (numSamples * 12));

        for (int s = 0; s < numSamples; ++s)
        {
            if (s > 0) { indicesRow << ","; valuesRow << ","; }
            indicesRow << juce::String (mLogBlockStartIndex + s);
            valuesRow  << juce::String (mLogBuffer.getSample (ch, s), 8);
        }
        indicesRow << "\n";
        valuesRow  << "\n";

        file.appendText (indicesRow);
        file.appendText (valuesRow);
    }
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
