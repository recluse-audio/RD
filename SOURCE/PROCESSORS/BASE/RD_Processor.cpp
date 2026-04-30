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

    doPrepareToPlay (sampleRate, samplesPerBlock);

    _fireLifecycleLog (LifecycleState::kPreparedToPlay);
}

void RD_Processor::doPrepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void RD_Processor::releaseResources()
{
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
    dir.getChildFile ("input_samples.csv") .deleteFile();
    dir.getChildFile ("output_samples.csv").deleteFile();
}

void RD_Processor::stopLogging()
{
    setIsLogging (false);
}

bool RD_Processor::doLogData()
{
    switch (mLifecycleState)
    {
        case LifecycleState::kPreparedToPlay:     return _logPrepareToPlay();
        case LifecycleState::kProcessBlockStart:  return _logProcessBlockStart();
        case LifecycleState::kProcessBlockEnd:    return _logProcessBlockEnd();
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

bool RD_Processor::_logPrepareToPlay()
{
    auto file = getDataLogOutputDirectory().getChildFile ("prepare_to_play.csv");

    juce::String contents;
    contents << "sampleRate,maxBlockSize\n";
    contents << juce::String (mSampleRate) << "," << juce::String (mBlockSize) << "\n";

    file.replaceWithText (contents);
    return true;
}

bool RD_Processor::_logProcessBlockStart()
{
    _writeBlockSamplesCsv ("input_samples.csv");
    return true;
}

bool RD_Processor::_logProcessBlockEnd()
{
    _writeBlockSamplesCsv ("output_samples.csv");
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

void RD_Processor::_writeBlockSamplesCsv (const juce::String& filename)
{
    const auto file        = getDataLogOutputDirectory().getChildFile (filename);
    const int  numChannels = mLogBuffer.getNumChannels();
    const int  numSamples  = mLogBuffer.getNumSamples();

    juce::String globalRow, localRow;
    globalRow.preallocateBytes (static_cast<size_t> (numSamples * 8));
    localRow .preallocateBytes (static_cast<size_t> (numSamples * 8));

    for (int s = 0; s < numSamples; ++s)
    {
        if (s > 0) { globalRow << ","; localRow << ","; }
        globalRow << juce::String (mLogBlockStartIndex + s);
        localRow  << juce::String (s);
    }
    globalRow << "\n";
    localRow  << "\n";

    file.appendText (globalRow);
    file.appendText (localRow);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        juce::String channelRow;
        channelRow.preallocateBytes (static_cast<size_t> (numSamples * 12));

        for (int s = 0; s < numSamples; ++s)
        {
            if (s > 0) channelRow << ",";
            channelRow << juce::String (mLogBuffer.getSample (ch, s), 8);
        }
        channelRow << "\n";

        file.appendText (channelRow);
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
