#pragma once
#include "Util/Juce_Header.h"

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

    bool read(juce::AudioBuffer<float>& readBuffer);
    bool write(juce::AudioBuffer<float>& writeBuffer);
    
private:
    // juce::File& mInputFile;
    // juce::File& mOutputFile;

    std::unique_ptr<juce::AudioFormatReader> mReader;
    std::unique_ptr<juce::AudioFormatWriter> mWriter;
    std::unique_ptr<juce::FileOutputStream> mOutputStream;

    juce::int64 mTotalSamples;
    juce::int64 mSamplesRead;

};