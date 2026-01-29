#pragma once
#include "Util/Juce_Header.h"
#include "Interpolator.h"
#include "BufferRange.h"
#include "Window.h"

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

    /**
     * @brief Compare two buffers at a specific sample index
     * @param buffer1 First buffer to compare
     * @param buffer2 Second buffer to compare
     * @param sampleIndex Index to compare at
     * @param channel Channel to compare (default 0)
     * @param tolerance Tolerance for comparison (default 0.001f)
     * @return std::tuple<bool, float, float> - (matches, value1, value2)
     */
    static std::tuple<bool, float, float> samplesMatchAtIndex(
        const juce::AudioBuffer<float>& buffer1,
        const juce::AudioBuffer<float>& buffer2,
        int sampleIndex,
        int channel = 0,
        float tolerance = 0.001f)
    {
        // Bounds checking
        if (sampleIndex < 0 || sampleIndex >= buffer1.getNumSamples() || sampleIndex >= buffer2.getNumSamples())
            return {false, 0.0f, 0.0f};

        if (channel < 0 || channel >= buffer1.getNumChannels() || channel >= buffer2.getNumChannels())
            return {false, 0.0f, 0.0f};

        float value1 = buffer1.getSample(channel, sampleIndex);
        float value2 = buffer2.getSample(channel, sampleIndex);
        float diff = std::abs(value1 - value2);

        bool matches = diff <= tolerance;

        return {matches, value1, value2};
    }

    // Cleaned up way to find the highest rms among the channels of audio data in a buffer
    static float getMaxRMS(juce::AudioBuffer<float>& buffer, double threshold = 0.0001)
    {
        juce::ignoreUnused(threshold); // TODO: implement threshold functionality
        float highestRMS = 0.f;
        for(int ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            float currentRMS = buffer.getRMSLevel(ch, 0, buffer.getNumSamples());
            if(currentRMS > highestRMS)
                highestRMS = currentRMS;

        }

        return highestRMS;
        
    }


	// Returns index with peak value
	static int getPeakIndex(juce::AudioBuffer<float>& buffer, int startIndex, int endIndex)
	{
		// if given out of bounds range, ignore the out of bounds stuff
		int rectifiedStartIndex = startIndex > 0 ? startIndex : 0;
		int rectifiedEndIndex = endIndex < buffer.getNumSamples() ? endIndex : buffer.getNumSamples() - 1;

        auto readPtr = buffer.getArrayOfReadPointers();

		int currentPeakIndex = rectifiedStartIndex;
		float currentPeakValue = readPtr[0][rectifiedStartIndex];

        for(int sampleIndex = rectifiedStartIndex; sampleIndex <= rectifiedEndIndex; sampleIndex++)
        {
			for(int ch = 0; ch < buffer.getNumChannels(); ch++)
			{

				if( readPtr[ch][sampleIndex] > currentPeakValue )
				{
					currentPeakValue = readPtr[ch][sampleIndex];
					currentPeakIndex = sampleIndex;
				}
			}

        }

		return currentPeakIndex;
	}

	// Returns expectedPeak if no value in range exceeds expectedPeakValue, otherwise returns actual peak index
	static int getPeakIndex(juce::AudioBuffer<float>& buffer, int startIndex, int endIndex, float expectedPeakValue)
	{
		int actualPeakIndex = getPeakIndex(buffer, startIndex, endIndex);
		float actualPeakValue = buffer.getSample(0, actualPeakIndex);

		if (actualPeakValue <= expectedPeakValue)
			return expectedPeakValue;

		return actualPeakIndex;
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

	//=========================
	// returns the value at given float position (between indices) and performs linear interpolation to return the approx val
	static float getLinearInterpolatedSampleAtIndex(juce::AudioBuffer<float>& buffer, float floatIndex)
	{
		int startIntIndex = (int)floatIndex;
		int nextIntIndex = startIntIndex + 1;
		// keep it in range
		if(nextIntIndex >= buffer.getNumSamples())
			nextIntIndex = nextIntIndex - buffer.getNumSamples();

		double sample1 = buffer.getSample(0, startIntIndex);
		double sample2 = buffer.getSample(0, nextIntIndex);
		double delta = floatIndex - (float)startIntIndex;

		float interpolatedSample = (float)Interpolator::linearInterp(sample1, sample2, delta);
		return interpolatedSample;
	}

	// throws assertion if range is... out of range.  Don't want hidden shortening going on
	// keep things in range before calling this
	static juce::dsp::AudioBlock<float> getRangeAsBlock(juce::AudioBuffer<float>& buffer, RD::BufferRange range)
	{
		// Keep this in range sample wise
		jassert (range.getStartIndex() >= 0
				&& range.getEndIndex() <= buffer.getNumSamples());

		auto startSample  = static_cast<size_t> (range.getStartIndex());
		auto numSamples   = static_cast<size_t> (range.getLengthInSamples());

		juce::dsp::AudioBlock<float> fullBlock(buffer);
		juce::dsp::AudioBlock<float> subBlock = fullBlock.getSubBlock(startSample, numSamples);
		// Wrap only the requested sample‐range and all channels
		return subBlock;
	}

	// Does not do anything special if block/buffer have different numChannels, same with valid range
	// only promises to not explode.
	// this is an "add" not a "copy", so block values are added on top of buffer values at those indices
	// read from block, 0 to range.length || write to buffer from range.start to range.end
	static bool writeBlockToBuffer(juce::AudioBuffer<float>& buffer, juce::dsp::AudioBlock<float>& block, RD::BufferRange range)
	{
		// trying to read outside buffer's numSamples
		if(range.getStartIndex() < 0 || range.getEndIndex() >= buffer.getNumSamples())
			return false;

		// TODO: handle varied channel config
		if(buffer.getNumChannels() != block.getNumChannels())
			return false;

		// range might be shorter than full block
		juce::dsp::AudioBlock<float> subBlock = block.getSubBlock(0, static_cast<size_t>(range.getLengthInSamples()));

		for(juce::int64 indexInRange = 0; indexInRange < range.getLengthInSamples(); indexInRange++)
		{
			juce::int64 indexInBuffer = indexInRange + range.getStartIndex();

			for(int ch = 0; ch < subBlock.getNumChannels(); ch++)
			{
				float blockSample = subBlock.getSample(ch, static_cast<int>(indexInRange));
				float bufferSample = buffer.getSample(ch, static_cast<int>(indexInBuffer));
				float newBufferSample = blockSample + bufferSample;
				buffer.setSample(ch, static_cast<int>(indexInBuffer), newBufferSample);
			}
		}

		return true;
	}

	//
	// Applies window to block of audio data. This assumes that the windows size/shape/period have been set and it is ready to go
	// doesn't do any checks to make sure these align in size so do that before if it is important.
	// You'll want to be sure the windows period is equal to the block length
	// TODO: This needs to handle partial windows
	static bool applyWindowToBlock(juce::dsp::AudioBlock<float>& block, Window& window)
	{
		if(window.getShape() == Window::Shape::kNone)
			return true;

		// TODO: 
		for(int sampleIndex = 0; sampleIndex < block.getNumSamples(); sampleIndex++)
		{
			float windowValue = window.getNextSample();
			for(int ch = 0; ch < block.getNumChannels(); ch++)
			{
				float blockSample = block.getSample(ch, sampleIndex);
				float windowedSample = blockSample * windowValue;
				block.setSample(ch, sampleIndex, windowedSample);
			}
		}

		return true; // TODO: this should be a void function
	}

	static bool applyWindowToBlock(juce::dsp::AudioBlock<float>& block, Window& window, float startingPhase)
	{
		juce::ignoreUnused(startingPhase); // TODO: implement phase offset functionality
		// TODO:
		for(int sampleIndex = 0; sampleIndex < block.getNumSamples(); sampleIndex++)
		{
			float windowValue = window.getNextSample();
			for(int ch = 0; ch < block.getNumChannels(); ch++)
			{
				float blockSample = block.getSample(ch, sampleIndex);
				float windowedSample = blockSample * windowValue;
				block.setSample(ch, sampleIndex, windowedSample);
			}
		}
		return true;
	}
};