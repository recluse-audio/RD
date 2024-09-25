#pragma once
#include "Util/Juce_Header.h"

/**
 * @brief An assortment of functions for filling buffers with amplitude values in various patters
 * 
 */

class BufferFiller
{
public:


    //////////////////////////
    // All indices set to 1.f
    static void fillWithAllOnes(juce::AudioBuffer<float>& bufferToFill)
    {
        bufferToFill.clear();
        auto writePtr = bufferToFill.getArrayOfWritePointers();

        for(int sampleIndex = 0; sampleIndex < bufferToFill.getNumSamples(); sampleIndex++)
        {
            for(int ch = 0; ch < bufferToFill.getNumChannels(); ch++)
            {
                writePtr[ch][sampleIndex] = 1.f;
            }

        }
    }

    /////////////////////////
    // Indices alternate values 0.f and 1.f (starting on 0.f)
    static void fillAlternatingZeroOne(juce::AudioBuffer<float>& bufferToFill)
    {
        bufferToFill.clear();
        auto writePtr = bufferToFill.getArrayOfWritePointers();

        // when to write a zero at an index, this will flip with each sample
        bool shouldWriteZero = true;

        for(int sampleIndex = 0; sampleIndex < bufferToFill.getNumSamples(); sampleIndex++)
        {
            float sampleValue = 1.f;
            if(shouldWriteZero)
                sampleValue = 0.f;

            for(int ch = 0; ch < bufferToFill.getNumChannels(); ch++)
            {
                writePtr[ch][sampleIndex] = sampleValue;
            }

            shouldWriteZero = !shouldWriteZero;
        }
    }

    //=======================
    // Value stored is equal to its index. Useful in testing
    static void fillIncremental(juce::AudioBuffer<float>& bufferToFill)
    {
        bufferToFill.clear();
        auto writePtr = bufferToFill.getArrayOfWritePointers();

        for(int sampleIndex = 0; sampleIndex < bufferToFill.getNumSamples(); sampleIndex++)
        {
            for(int ch = 0; ch < bufferToFill.getNumChannels(); ch++)
            {
                writePtr[ch][sampleIndex] = (float)sampleIndex;
            }

        }
    }


    //====================
    //
    static void generateHanning(juce::AudioBuffer<float>& bufferToFill)
    {
        bufferToFill.clear();

        auto writePtr = bufferToFill.getArrayOfWritePointers();
        int numSamples = bufferToFill.getNumSamples() - 1;  // why -1 I am not sure yet
        
        for(int sampleIndex = 0; sampleIndex <= numSamples; sampleIndex++)
        {
            writePtr[0][sampleIndex] = 0.5f * (1.0f - std::cos(2.0f * M_PI * sampleIndex / (numSamples)));
        }
    }

    //===========================================
    /** Loads an AudioBuffer from a .wav file */
    static bool loadFromWavFile(const juce::File& wavFile, juce::AudioBuffer<float>& buffer)
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(wavFile));
        if (reader.get() == nullptr)
        {
            DBG("Failed to create reader for WAV file.");
            return false;
        }

        buffer.setSize(static_cast<int>(reader->numChannels), static_cast<int>(reader->lengthInSamples));
        reader->read(&buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
        DBG("Successfully loaded buffer from WAV file.");
        return true;
    }

    //=================================
    /** Loads an AudioBuffer from a .json file containing amplitude values */
    static bool loadFromJsonFile(const juce::File& jsonFile, juce::AudioBuffer<float>& buffer)
    {
        juce::FileInputStream inputStream(jsonFile);
        if (!inputStream.openedOk())
        {
            DBG("Failed to open JSON file.");
            return false;
        }

        auto jsonParsed = juce::JSON::parse(inputStream);
        if (jsonParsed.isVoid() || !jsonParsed.isArray())
        {
            DBG("Failed to parse JSON file or JSON is not an array.");
            return false;
        }

        juce::Array<juce::var> jsonArray = *jsonParsed.getArray();
        int numSamples = jsonArray.size();
        buffer.setSize(1, numSamples); // Assuming mono channel for JSON data
        buffer.clear();

        for (int i = 0; i < jsonArray.size(); ++i)
        {
            if (jsonArray[i].isDouble())
            {
                float sample = jsonArray[i];
                buffer.setSample(0, i, sample);
            }
            else
            {
                DBG("Invalid amplitude value in JSON file at index " + juce::String(i));
                return false;
            }
        }

        DBG("Successfully loaded buffer from JSON file.");
        return true;
    }
};




