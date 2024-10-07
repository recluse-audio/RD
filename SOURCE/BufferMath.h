/**
 * @file BufferMath.h
 * @author Artie Evans
 * @brief 
 * @version 0.1
 * @date 2024-09-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#pragma once
#include "Util/Juce_Header.h"

/**
 * @brief Functions that take buffers by ref or return buffers.
 * At first implementation, this is for YIN difference, cum diff, threshold, etc., calculations
 * May be used for other stuff in the future.  Who knows!
 * 
 */

class BufferMath
{
public:


    //================================================================
    /**
     * @brief Difference function, i.e. step 2 of YIN!
     * As I understand it, this tests 
     * the first half of the ioBuffer against the second half by finding the inverse of the ACF at each lag time?
     * 
     * This is derived from farbot/pidato implementation and a yin video by a youtuber "v for science"
     * This is a very coder friendly version of this for now, and optimizing when a change is PROVEABLY better in some way
     * K.I.S.S., Don't go thinking you're Curtis Roads, Ryan.
     * 
     * 
     * @param ioBuffer // audio buffer that is being 
     * @param differenceBuffer // gets cleared, then filled with the difference values at lag times equal to each index
     * @param max_lag // This better be less than or ideally equal to 1/2 your ioBuffer or you get nothing
     */
    // 
    static void yin_difference(juce::AudioBuffer<float>& ioBuffer, juce::AudioBuffer<float>& differenceBuffer, int max_lag)
    {
        // TODO: reconsider this safety/optimization.  Is it helping?
        // depending on the diff buffer being 1/2 size of ioBuffer
        // if( (max_lag + max_lag) != ioBuffer.getNumSamples() )
        //     return;
        


        // using 'tau' in reference to YIN paper
        for(int tau = 0; tau < max_lag; tau++)
        {
            float difference = 0.f;
            for(int i = 0; i < max_lag; i++)
            {
                float sample = ioBuffer.getSample(0, i);
                float shiftedSample = ioBuffer.getSample(0, i+tau);
                float delta = sample - shiftedSample;
                difference += (delta * delta);
            }
            
            differenceBuffer.setSample(0, tau, difference);
        }    
    }

    //================================================================
    /**
     * @brief Performs the Cumulative Mean Normalized Difference function seen in yin #3
     * 
     * The equation calls to sum 'j=1' to 'tau' for every difference index.  Instead we keep a runningSum to avoid re-addition of diff indices we previously applied
     * @param differenceBuffer // a buffer with each sample corresponding with a difference value at lag times equal to each index
     * @param normalizedDifferenceBuffer // 
     */
    static void yin_normalized_difference( juce::AudioBuffer<float>& differenceBuffer, juce::AudioBuffer<float>& normalizedDifferenceBuffer)
    {
        normalizedDifferenceBuffer.setSample(0, 0, 1.f);
        float runningSum = 0.f; // let's us cumulative sum of the differences up to each tau without recalculating
        for (int tau = 1; tau < differenceBuffer.getNumSamples(); tau++)
        {
            auto differenceValue = differenceBuffer.getSample(0, tau);
            runningSum += differenceValue;
            auto cmndValue = differenceValue * (tau / runningSum);
            normalizedDifferenceBuffer.setSample(0, tau, cmndValue);
        }
    }


    /**
     * @brief Returns the smallest value of tau that gives min of cumulative difference that is above the threshold.
     * 
     */
    static int yin_absolute_threshold(juce::AudioBuffer<float>& normalizedDiffBuffer, float threshold)
    {

    }

private:

};