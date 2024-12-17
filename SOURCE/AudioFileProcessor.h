#pragma once
#include "Util/Juce_Header.h"


/**
 * @brief Info about an input file
 * 
 */
struct FileData
{
    int kNumSamples = 0;
    int kNumChannels = 0;
};
 
/**
 * @brief This class reads an existing .wav file buffer by buffer,
 * then writes it buffer by buffer.
 * 
 * 
 * 
 */

class AudioFileProcessor
{
public:
    AudioFileProcessor();
    ~AudioFileProcessor();

    bool init(juce::File& inputFile, juce::File& outputFile);
    void reset();



    // TODO: I want to be able to read from the outputFile as well for testing.  
    // I could do this generically to allow reading of a range from whatever file, 
    // and we'd just instantiate temp reader/writer/stream

    bool read(juce::AudioBuffer<float>& readBuffer, int startInFile);


    bool write(juce::AudioBuffer<float>& writeBuffer, int startPos);
    
    const int getNumReadSamples();
private:
    // juce::File& mInputFile;
    // juce::File& mOutputFile;

    std::unique_ptr<juce::AudioFormatReader> mReader;
    std::unique_ptr<juce::AudioFormatWriter> mWriter;
    std::unique_ptr<juce::FileOutputStream> mOutputStream;

    juce::int64 mTotalSamples = 0;
    juce::int64 mSamplesRead = 0;

    int mNumInputChannels = 0; 
    int mNumOutputChannels = 0;

};