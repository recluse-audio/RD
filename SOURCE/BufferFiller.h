#pragma once
#include "Util/Juce_Header.h"

/**
 * @brief An assortment of functions for filling buffers with amplitude values in various patters
 * 
 */

class BufferFiller
{
public:

    /**
     * @brief You pass in the short name of the file you want in the SUBMODULES/RD/TESTS/GOLDEN/ directory 
     * and this returns the full path with the correct CWD.
     * 
     * Seems silly, but I'm finding I use these alot for testing, but I am also test-driving
     * 
     * Not to mention, I'd like to build off of these "golden" files, so this convenience function will encourage that. 
     * 
     * @param fileName 
     * @return juce::String, the proper filename for the resulting output file
     * 
     * This is kind of the inverse of 'getTestOutputPath()' in BufferWriter.  In many ways these two classes are mirrors
     */
    // 
    static juce::String getGoldenFilePath(juce::String fileName)
    {
        // Define the directory and output file path
        juce::File currentDir = juce::File::getCurrentWorkingDirectory(); // this works when called from root dir of repo
        juce::String relativePath = "/SUBMODULES//RD/TESTS/GOLDEN/"; 

        return currentDir.getFullPathName() + relativePath + fileName;
    }

    //======================
    //
    static void fillFromArray(juce::AudioBuffer<float>& buffer, const std::vector<float>& array)
    {
        buffer.setSize(1, static_cast<int>(array.size()));

        // Fill the buffer with the input array values
        for (int i = 0; i < array.size(); ++i)
        {
            buffer.setSample(0, i, array[i]);
        }
    }

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

    //=======================
    // fills buffer with "value" argument
    static void fillWithValue(juce::AudioBuffer<float>& bufferToFill, float value)
    {
        bufferToFill.clear();
        auto writePtr = bufferToFill.getArrayOfWritePointers();

        for(int sampleIndex = 0; sampleIndex < bufferToFill.getNumSamples(); sampleIndex++)
        {
            for(int ch = 0; ch < bufferToFill.getNumChannels(); ch++)
            {
                writePtr[ch][sampleIndex] = value;
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
    static bool loadFromJsonFile(const juce::File& jsonFile, juce::AudioBuffer<float>& buffer, const juce::String& key = "Channel_0")
    {
        juce::FileInputStream inputStream(jsonFile);
        if (!inputStream.openedOk())
        {
            DBG("Failed to open JSON file.");
            return false;
        }

        auto jsonParsed = juce::JSON::parse(inputStream);
        if (jsonParsed.isVoid() || !jsonParsed.isObject())
        {
            DBG("Failed to parse JSON file or JSON is not an array.");
            return false;
        }

        auto keyID = juce::Identifier(key);
        // Ensure the JSON root is an object and find the array by the given key
        juce::var jsonObject = jsonParsed;
        if (!jsonObject.hasProperty(keyID))
        {
            DBG("Key not found in JSON file: " + key);
            return false;
        }

        // Check that the key contains an array
        auto jsonValue = jsonObject[keyID];
        if (!jsonValue.isArray())
        {
            DBG("Value associated with the key is not an array.");
            return false;
        }

        // Get the array of values
        juce::Array<juce::var> jsonArray = *jsonValue.getArray();
        int numSamples = jsonArray.size();
        buffer.setSize(1, numSamples); // Assuming mono channel for JSON data
        buffer.clear();

        // Populate the buffer with data from the JSON array
        for (int i = 0; i < jsonArray.size(); ++i)
        {
            if (jsonArray[i].isDouble())
            {
                double sample = jsonArray[i];
                for (int ch = 0; ch < buffer.getNumChannels(); ch++)
                    buffer.setSample(ch, i, (float)sample);
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




