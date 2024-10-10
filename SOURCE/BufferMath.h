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
     * For the sake of matching the language of YIN paper as much as possible, please forgive me for abbreviating in ways I otherwise would not.
     * God forgive this petty soul.
     * 
     * @param cmndBuffer:  The "Cumulative Mean Normalized Difference Buffer".  You should pass in the buffer that was filled doing 'yin_normalized_difference'
     */
    static int yin_absolute_threshold(juce::AudioBuffer<float>& cmndBuffer, float threshold)
    {
        int cmndSize = cmndBuffer.getNumSamples();

        /* Search through the vector of cumulative mean values, and look for ones that are over the threshold 
        * The first two positions in vector are always 1 so start at the third (index 2) */
        for (int tau = 2; tau < cmndSize ; tau++) 
        {
            // Tau is the sample index of the first normalized difference under threshold
            if (cmndBuffer.getSample(0, tau) < threshold) 
                return tau;
        }

        // Didn't find anything, Impl should handle 0 result
        return 0;
    }

    /**
     * @brief Parabolic interpolation of normalized difference at tau and its immediate neighbors.  
     * This is in cases where the best tau is not a multiple of the period.
     * 
     * NOTE: I am passing in the RAW difference buffer, not cmndBuffer, per YIN paper recommendation
     * This diverges from the 'pidato' implementation that first inspired the endeavor
     */
    static float yin_parabolic_interpolation(juce::AudioBuffer<float>& differenceBuffer, int tauEstimate)
    {
        if(tauEstimate <= 0)
            return 0.f; // we aint doing negative estimates

        int x0 = tauEstimate - 1;
        int x2 = tauEstimate + 1;

        // keep things in range
        if(x0 < 0)
            x0 = 0;
        if(x2 > differenceBuffer.getNumSamples())
            x2 = tauEstimate;

        float y0 = differenceBuffer.getSample(0, x0);
        float y1 = differenceBuffer.getSample(0, tauEstimate);
        float y2 = differenceBuffer.getSample(0, x2);

        // using common variables from quadratic function xmin = -b/2a;
        float b = y2 - y0;
        float a = (2 * y1) - y2 - y0;
        if(a == 0.f) // don't divide by zero
            return tauEstimate; 
        
        // this is not "-b" like you might guess, can't articulate why right now
        return (float)tauEstimate + ( b / ( 2 * a));


        // TODO: Write this interpolation in a math function abstracted from this
    }

private:

};