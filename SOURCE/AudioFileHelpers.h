/** 
 * Made by Ryan Devens on 2024-12-16
 */

#pragma once
#include "UTIL/Juce_Header.h"

/**
 * @brief A class of static functions for reading from audio files.
 * 
 */
class AudioFileHelpers
{
public:


    /**
     * @brief Reads length from inputFile into outputBuffer (if it is a wav file)
     * 
     * This adds samples not copies
     * 
     * @param inputFile 
     * @param outputBuffer 
     * @param startSample 
     * 
     * No param for length.  For simplicity's sake it is the size of the outputBuffer passed in.  Sorry!
     * If the length of the buffer extends past the end of the inputFile it won't explode but it will just stop 
     * adding values to the outputBuffer.
     * 
     * This does not resize buffer to match numChannels of inputFile, but again won't explode if they don't match
     */
    static void readRange(juce::File& inputFile, juce::AudioBuffer<float>& outputBuffer, int startInFile)
    {
        // Initialize the AudioFormatManager
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> fileReader(formatManager.createReaderFor(inputFile));
        if(fileReader == nullptr)
            return; // we've failed to provide a valid inputFile

        // Read data into the output buffer
        int numChannels = outputBuffer.getNumChannels();
        int numSamples = outputBuffer.getNumSamples();

        bool useRightChannel = numChannels >= 2 ? true : false;

        fileReader->read(&outputBuffer, 0, numSamples, startInFile, true, useRightChannel);


    }

    //
    static const int getFileLengthInSamples(juce::File& inputFile)
    {
        // Initialize the AudioFormatManager
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> fileReader(formatManager.createReaderFor(inputFile));
        if(fileReader == nullptr)
            return -1; // we've failed to provide a valid inputFile

        return static_cast<const int>(fileReader->lengthInSamples);
    }

    //
    static const double getFileSampleRate(juce::File& inputFile)
    {
        // Initialize the AudioFormatManager
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> fileReader(formatManager.createReaderFor(inputFile));
        if(fileReader == nullptr)
            return -1; // we've failed to provide a valid inputFile

        return fileReader->sampleRate;
    }
};