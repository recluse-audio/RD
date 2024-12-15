#include "AudioFileProcessor.h"

//======================
AudioFileProcessor::AudioFileProcessor()
{
    //reset();
}

//======================
AudioFileProcessor::~AudioFileProcessor()
{
    reset();
}

//======================
bool AudioFileProcessor::init(juce::File& inputFile, juce::File& outputFile)
{
    // Initialize the AudioFormatManager
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    // Create the reader for the input file
    mReader.reset(formatManager.createReaderFor(inputFile));
    if (mReader == nullptr)
        return false;

    //
    mTotalSamples = mReader->lengthInSamples;

    // Create output file stream
    mOutputStream = std::make_unique<juce::FileOutputStream>(outputFile);
    if (mOutputStream == nullptr || !mOutputStream->openedOk())
        return false;

    // Create the writer
    juce::WavAudioFormat wavFormat;

    mWriter.reset(wavFormat.createWriterFor(
        mOutputStream.get(),               // Output stream
        mReader->sampleRate,               // Sample rate
        mReader->numChannels,              // Number of channels
        24,                               // Bits per sample
        {},                               // No metadata
        0));   // No compression); 

    return mWriter != nullptr;
}

//======================
void AudioFileProcessor::reset()
{
    mOutputStream.release();
    mWriter.reset();
    mReader.reset();
    mTotalSamples = 0;
    mSamplesRead = 0;
}

//======================
bool AudioFileProcessor::read(juce::AudioBuffer<float>& readBuffer)
{
    
}

//======================
bool AudioFileProcessor::write(juce::AudioBuffer<float>& writeBuffer)
{
    
}

//=======================
const int AudioFileProcessor::getNumReadSamples()
{
    return mTotalSamples;
}