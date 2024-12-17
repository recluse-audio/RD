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
    static bool buffersAreIdentical(juce::AudioBuffer<float>& buffer1, juce::AudioBuffer<float>& buffer2)
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

                if(sample1 != sample2)
                    return false;
            }
        }

        return true;
    }

};