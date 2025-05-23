#pragma once
#include "Util/Juce_Header.h"


/**
 * @brief An assortment of functions for comparing buffers in frequent ways.
 * 
 * Stuff that does not fall under BufferFiller or BufferMath or juce::AudioBlock for that matter
 * 
 */
class BufferHelper
{
public:

    /**
     * @brief Compares two buffers to determine if they are identical.
     * Meaning they are same numSamples/numChannels and the sample values stored in them are identical.
     */
    static bool buffersAreIdentical(juce::AudioBuffer<float>& buffer1, juce::AudioBuffer<float>& buffer2, float threshold = 0.001f)
    {
        if(buffer1.getNumChannels() != buffer2.getNumChannels())
            return false;

        if(buffer1.getNumSamples() != buffer2.getNumSamples())
            return false;

        for(int sampleIndex = 0; sampleIndex < buffer1.getNumSamples(); sampleIndex++)
        {

            for(int ch = 0; ch < buffer1.getNumChannels(); ch++)
            {
                auto sample1 = buffer1.getSample(ch, sampleIndex);
                auto sample2 = buffer2.getSample(ch, sampleIndex);

				float difference = std::abs(sample1 - sample2);
                if(difference >= threshold)
                    return false;
            }
        }

        return true;
    }

    // Cleaned up way to find the highest rms among the channels of audio data in a buffer
    static float getMaxRMS(juce::AudioBuffer<float>& buffer, double threshold = 0.0001)
    {

        float highestRMS = 0.f;
        for(int ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            float currentRMS = buffer.getRMSLevel(ch, 0, buffer.getNumSamples());
            if(currentRMS > highestRMS)
                highestRMS = currentRMS;

        }

        return highestRMS;
        
    }

    static bool isSilent(juce::AudioBuffer<float>& buffer, double threshold = 0.0001)
    {
       bool isSilent = BufferHelper::getMaxRMS(buffer) < threshold ? true : false;
       return isSilent;
    }


    // Takes the given buffer, chunks it up at blockSize, and calls the processor's processBlock until finished filling the buffer 
    // This is designed for long buffers on which you'd call processBlock many times, it would be silly to pass a buffer smaller than block size
    static void processBuffer(juce::AudioBuffer<float>& buffer, juce::AudioProcessor& processor, int blockSize)
    {
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();
        if(numSamples < blockSize)
            return;

        int blockIndex = 0;

        juce::AudioBuffer<float> processBuffer(numChannels, blockSize);
        juce::MidiBuffer midiBuffer;

        while(blockIndex < numSamples)
        {   
            processBuffer.clear();
            
            for(int ch = 0; ch < numChannels; ch++)
            {
                processBuffer.copyFrom(ch, 0, buffer, ch, blockIndex, blockSize);
            }

            processor.processBlock(processBuffer, midiBuffer);

            for(int ch = 0; ch < numChannels; ch++)
            {
                buffer.copyFrom(ch, blockIndex, processBuffer, ch, 0, blockSize);
            }

            blockIndex = blockIndex + blockSize;
        }

    }


    //=======================
    // gets a wrapped version of the 'indexToWrap' argument, according to size of buffer
    static int getWrappedIndex(juce::AudioBuffer<float>& buffer, int indexToWrap)
    {
        int numSamples = buffer.getNumSamples();
        int index = 0;

        if(indexToWrap < 0)
            index = indexToWrap + numSamples;
        else if(indexToWrap >= numSamples)
            index = indexToWrap - numSamples;
        else
            index = indexToWrap;

        return index;
    }
};