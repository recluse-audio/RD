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


    //=======================
    // 
    static void generateSine(juce::AudioBuffer<float>& bufferToFill)
	{
		bufferToFill.clear();
		auto numChannels = bufferToFill.getNumChannels();
		float numSamples = bufferToFill.getNumSamples();
		
		auto writeBuff = bufferToFill.getArrayOfWritePointers();
    	for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
		{
            auto sample = std::sin( (sampleIndex * juce::MathConstants<float>::twoPi) / numSamples );
            for(int channel = 0; channel < numChannels; channel++)
            {
				writeBuff[channel][sampleIndex] = sample;
			}
		}

	}
	
    //=======================
    // Fills buffer with multiple cycles of a sine wave. You figure out how many it will be at this period length
    static void generateSineCycles(juce::AudioBuffer<float>& bufferToFill, int period)
	{
		bufferToFill.clear();
		auto numChannels = bufferToFill.getNumChannels();
		float numSamples = bufferToFill.getNumSamples();
		
		auto writeBuff = bufferToFill.getArrayOfWritePointers();

        // this will iterate like the sampleIndex, except it will wrap around the period
        // presumably some number other than 1, otherwise why bother with this function?  Use 'generateSine()'
        int writePos = 0;
    	for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
		{
            float sample = std::sin( ((float)writePos * juce::MathConstants<float>::twoPi) / (float)period );

            writePos++;
            if(writePos >= period)
                writePos = writePos - period;

            for(int channel = 0; channel < numChannels; channel++)
            {
				writeBuff[channel][sampleIndex] = sample;
			}
		}

	}

    //=======================
	// Generates a sine wave in a stereo buffer that has different phase in each channel
	static void generateStereoSineWithPhaseVariance(juce::AudioBuffer<float>& bufferToFill)
	{
		bufferToFill.clear();
		auto numChannels = bufferToFill.getNumChannels();
		float numSamples = bufferToFill.getNumSamples();
		
		if(numChannels != 2)
			return;
		
		auto writeBuff = bufferToFill.getArrayOfWritePointers();

		for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
		{
			auto leftSample = std::sin( (sampleIndex * juce::MathConstants<float>::twoPi) / numSamples );
			auto rightSample = std::sin( (sampleIndex * juce::MathConstants<float>::twoPi * 2.f ) / numSamples );

			writeBuff[0][sampleIndex] = leftSample;
			writeBuff[1][sampleIndex] = rightSample;
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
                double sample = jsonArray[i];
                buffer.setSample(0, i, (float)sample);
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




