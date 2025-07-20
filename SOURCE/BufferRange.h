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
		// doing this to the underlying range to prevent catastrophe, but really in this case the range is empty
		mRange.setStart(0);
		mRange.setEnd(0);
	}

	BufferRange(juce::int64 start, juce::int64 end)
	{
		setStartIndex(start);
		setEndIndex(end);
	}

	void setStartIndex(juce::int64 startIndex)
	{
		mRange.setStart(startIndex);
		setIsEmpty(false);
	}

	void setEndIndex(juce::int64 endIndex)
	{
		mRange.setEnd(endIndex);
		setIsEmpty(false);
	}

	
	void setLengthInSamples(juce::int64 lengthInSamples)
	{
		mRange.setLength(lengthInSamples - (juce::int64)1);

		lengthInSamples > 0 ? setIsEmpty(false) : setIsEmpty(true);
	}

	juce::int64 getStartIndex() const {	return mRange.getStart();	}
	juce::int64 getEndIndex() const	{	return mRange.getEnd();	}
	juce::int64 getLengthInSamples() const	{	return mRange.getLength() + (juce::int64)1;	}
	const juce::Range<juce::int64>& getRange() const { return mRange; }

	// This exists to handle the fact that a range that starts and ends with [0] corresponds with a length of 1 sample
	// Using negative is not deisreable either, because it may be used in lookahead situations and I don't want to prohibit that
	void setIsEmpty(bool isEmpty) { mIsEmpty = isEmpty; }
	bool isEmpty() { return mIsEmpty; }


private:
	bool mIsEmpty = true;
	juce::Range<juce::int64> mRange;


};


} // namespace RD