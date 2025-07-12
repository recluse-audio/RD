/**
 * @file BufferRange.h
 * @author Ryan Devens
 * @brief 
 * @version 0.1
 * @date 2025-07-02
 * 
 * @copyright Copyright (c) 2025
 * 
 */


#pragma once
#include "Util/Juce_Header.h"







namespace RD
{
 /**
  * @brief This class exists because the juce::Range is not ideal for referencing ranges of sample indices in juce buffers.  
  * I thought juce::dsp::AudioBlock might fill this gap, but I was left wanting. 
  * 
  * The fundamental issue is that a juce::Range of length 20 will also have an end of 20. 
  * A juce::AudioBuffer with size of 20 samples would end at [19].  
  * 
  * This has led to countless dropped samples and "range.getLength() + 1 " situations. 
  * 
  * Plus, I am sick of casting things to juce::int64
  * 
  * This class manages the juce::Range and converts its stored values in a manner that makes sense when working with buffers
  * As a rule, the juce::Range's start/end will be accurate.  
  * 
  */

class BufferRange
{
public:

	BufferRange()
	{
		mRange.setStart(0);
		mRange.setEnd(0);
	}

	void setStartIndex(juce::int64 startIndex)
	{
		mRange.setStart(startIndex);
	}

	void setEndIndex(juce::int64 endIndex)
	{
		mRange.setEnd(endIndex);
	}

	
	void setLengthInSamples(juce::int64 lengthInSamples)
	{
		mRange.setLength(lengthInSamples - (juce::int64)1);
	}

	juce::int64 getStartIndex()	{	return mRange.getStart();	}
	juce::int64 getEndIndex()	{	return mRange.getEnd();	}
	juce::int64 getLengthInSamples()	{	return mRange.getLength() + (juce::int64)1;	}

	const juce::Range<juce::int64>& getRange() { return mRange; }

private:

	juce::Range<juce::int64> mRange;


};


} // namespace RD