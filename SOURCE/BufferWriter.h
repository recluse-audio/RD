/**
 * @file BufferWriter.h
 * @author Artie Evans
 * @brief 
 * @version 0.1
 * @date 2024-09-29
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once
#include "Util/Juce_Header.h"

/**
 * @brief Writing Buffers to various files and formats
 * 
 */

class BufferWriter
{
public:

    /**
     * @brief Get the correct path to where files should be output to when testing.
     * 
     * Not just helpful in automated testing, I've used this for debugging also
     * 
     * @param fileName 
     * @return juce::String, the proper filename for the resulting output file
     */
    // 
    static juce::String getTestOutputPath(juce::String fileName)
    {
        // Define the directory and output file path
        juce::File currentDir = juce::File::getCurrentWorkingDirectory(); // this works when called from root dir of repo
        juce::String relativePath = "/SUBMODULES//RD/TESTS/OUTPUT/"; 

        return currentDir.getFullPathName() + relativePath + fileName;
    }


    /**
     * @brief 
     * 
     * @param buffer 
     * @param outputFile 
     */
    static void writeToJson(juce::AudioBuffer<float>& buffer, const juce::File& outputFile)
    {
        // Create a JSON object
        juce::DynamicObject::Ptr jsonObject = new juce::DynamicObject();
        
        // Iterate through each channel in the buffer
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            // Create an array to hold the channel data
            juce::Array<juce::var> channelData;
            
            // Get the buffer data for this channel
            const float* channelSamples = buffer.getReadPointer(channel);
            
            // Iterate over the buffer length and store each sample in the array
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                channelData.add(juce::var(channelSamples[i]));
            }
            
            // Store the channel data in the JSON object, using the channel number as the key
            jsonObject->setProperty("Channel_" + juce::String(channel), juce::var(channelData));
        }

        // Convert the JSON object to a juce::var so we can serialize it
        juce::var jsonVar(jsonObject);

        // Write the JSON data to the output file
        juce::String jsonString = juce::JSON::toString(jsonVar);
        outputFile.replaceWithText(jsonString);
    }

    static void writeToCSV(juce::AudioBuffer<float>& buffer, const juce::File& outputFile)
    {

    }

private:

};