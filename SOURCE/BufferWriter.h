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

enum class BufferWriterResult
{
    kSuccess,
    kPathError,
    kFileError,
    kStreamError
};

/**
 * @brief Writing Buffers to various files and formats
 * 
 */

class BufferWriter
{
public:

    enum class Result
    {
        kSuccess,
        kPathError,
        kFileError,
        kStreamError
    };

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


    //=====================
    static BufferWriter::Result writeToCSV(juce::AudioBuffer<float>& buffer, const juce::String& csvPath)
    {
        juce::Array<juce::StringArray> csvData;

        // do channels first to get headers in csv
        juce::StringArray channelHeaders;
        channelHeaders.add("Sample Index");
        for(int ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            juce::String channelString = "Channel " + juce::String(ch);
            channelHeaders.add(channelString);
        }
        csvData.add(channelHeaders);
        //

        // Write sample values, each sampleIndex is a row in the csv.
        for(int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); sampleIndex++)
        {
            juce::StringArray sampleRow;

            juce::String indexString(sampleIndex);
            sampleRow.add(indexString);

            for(int ch = 0; ch < buffer.getNumChannels(); ch++)
            {
                auto sampleValue = buffer.getSample(ch, sampleIndex);
                juce::String sampleValueString(sampleValue);
                sampleRow.add(sampleValueString); 
            }

            csvData.add(sampleRow);

        }

        // Make file, start output stream
        auto csvFile = std::make_unique<juce::File>(csvPath);
        if(csvFile->exists())
            csvFile->deleteFile();

        csvFile->create();
        if(!csvFile->exists())
            return BufferWriter::Result::kFileError;
            
        auto fileStream = std::make_unique<juce::FileOutputStream>(*csvFile.get());
        if(fileStream->failedToOpen())
            return BufferWriter::Result::kStreamError;

        for(const auto& row : csvData)
        {
            // Join the row's columns with commas
            juce::String line = juce::StringArray(row).joinIntoString(",");
            fileStream->writeText(line + "\n", false, false, "\n");
        }
        
        return BufferWriter::Result::kSuccess;

    }

private:

};