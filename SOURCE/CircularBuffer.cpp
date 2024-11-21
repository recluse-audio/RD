#include "CircularBuffer.h"

CircularBuffer::CircularBuffer()
{
    mCircularBuffer.clear();
    mCircularBuffer.setSize(2, 48000);
}

//=======================
CircularBuffer::~CircularBuffer()
{

}

//=======================
void CircularBuffer::setSize(int numChannels, int numSamples)
{
    mCircularBuffer.clear();
    mCircularBuffer.setSize(numChannels, numSamples);
    mWritePos = 0;
    mReadPos = 0;
}

//=======================
const int CircularBuffer::getNumSamples()
{
    return mCircularBuffer.getNumSamples();
}

//=======================
const int CircularBuffer::getNumChannels()
{
    return mCircularBuffer.getNumChannels();
}

//=======================
const int CircularBuffer::getWritePos()
{
    return mWritePos;
}
//=======================
bool CircularBuffer::pushBuffer(juce::AudioBuffer<float>& buffer)
{
    bool success = false;
    success = _writeToCircularBuffer(buffer);
    return success;
}

//=======================
bool CircularBuffer::pushValue(int length, float value, int ch, bool incrementWritePos)
{
    bool success = false;
    success = _writeToCircularBuffer(length, value, ch, incrementWritePos);
    return success;
}

//=======================
bool CircularBuffer::popBuffer(juce::AudioBuffer<float>& buffer)
{
    bool success = false;
    success = _readFromCircularBuffer(buffer, mReadPos);
    _incrementReadPosition(buffer.getNumSamples());
    return success;
}

//=======================
//
bool CircularBuffer::readRange(juce::AudioBuffer<float>& buffer, int startingReadIndex)
{
    bool success = false;
    success = _readFromCircularBuffer(buffer, startingReadIndex);
    return success;
}

//=======================
//
// juce::dsp::AudioBlock<float> CircularBuffer::getAudioBlock(juce::dsp::AudioBlock<float>& blockOne, 
//                                                             juce::dsp::AudioBlock<float>& blockTwo,
//                                                              int readPos, int length)
// {
//     juce::dsp::AudioBlock<float> audioBlock(mCircularBuffer);

//     bool blockWillWrap = (readPos + length) >= (mCircularBuffer.getNumSamples() - 1) ? true : false;
    
//     if(blockWillWrap)
//     {
//         // how far we can read from before we need to wrap the readIndex.  '-1' b/c we start at [0]
//         int blockOneLength = mCircularBuffer.getNumSamples() - 1 - readPos;
//         // length left after blockOneLength.  Need '-1' ?
//         int blockTwoLength = length - blockOneLength;

//         auto subBlockOne = audioBlock.getSubBlock((size_t)readPos, (size_t)blockOneLength);
//         auto subBlockTwo = audioBlock.getSubBlock(0, (size_t)blockTwoLength);

//         // add sub-blocks to outblock
//         destinationBlock.getSubBlock(0, (size_t)blockOneLength).copyFrom(subBlockOne);
//         destinationBlock.getSubBlock((size_t)blockOneLength, blockTwoLength).copyFrom(subBlockTwo);
//     }
//     else
//     {
//         destinationBlock = audioBlock.getSubBlock((size_t)readPos, (size_t)length);
//     }

//     return destinationBlock;
// }

//=======================
//
juce::dsp::AudioBlock<float> CircularBuffer::popAudioBlock(int length)
{
    juce::dsp::AudioBlock<float> audioBlock(mCircularBuffer);
    auto subBlock = audioBlock.getSubBlock((size_t)mReadPos, (size_t)length);
}


//=======================
//
bool CircularBuffer::_writeToCircularBuffer(juce::AudioBuffer<float>& buffer)
{

    if(buffer.getNumChannels() != mCircularBuffer.getNumChannels())
        return false;

    for(int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); sampleIndex++)
    {
        for(int ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            auto sample = buffer.getSample(ch, sampleIndex);
            mCircularBuffer.setSample(ch, mWritePos, sample);

        }

        mWritePos++;
        if(mWritePos >= mCircularBuffer.getNumSamples())
            mWritePos = mWritePos - mCircularBuffer.getNumSamples();
    }

    return true;

}


//=======================
//
bool CircularBuffer::_writeToCircularBuffer(int length, float value, int channel, bool incrementWritePos)
{
    // attempting to write longer than the circular buffer won't blow up, but is pointless
    // if(length >= mCircularBuffer.getNumSamples())
    //     return false;

    for(int sampleIndex = 0; sampleIndex < length; sampleIndex++)
    {
        mCircularBuffer.setSample(channel, mWritePos, value);

        if(incrementWritePos)
            mWritePos++;

        if(mWritePos >= mCircularBuffer.getNumSamples())
            mWritePos = mWritePos - mCircularBuffer.getNumSamples();
    }

    return true;

}


//=======================
//
bool CircularBuffer::_readFromCircularBuffer(juce::AudioBuffer<float>& buffer, int startingReadIndex)
{
    if(buffer.getNumChannels() != mCircularBuffer.getNumChannels())
        return false;

    auto readPos = startingReadIndex;

    for(int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); sampleIndex++)
    {
        for(int ch = 0; ch < buffer.getNumChannels(); ch++)
        {
            auto sample = mCircularBuffer.getSample(ch, readPos);
            buffer.setSample(ch, sampleIndex, sample);

        }

        readPos++; // note this is not mReadPos, don't call _incrementReadPosition()
        if(readPos >= mCircularBuffer.getNumSamples())
            readPos = readPos - mCircularBuffer.getNumSamples();
    }

    return true;
}


//======================
//
void CircularBuffer::_incrementReadPosition(int numSamples)
{
    int newReadPos = mReadPos + numSamples;

    if(newReadPos >= mCircularBuffer.getNumSamples())
        newReadPos = newReadPos - mCircularBuffer.getNumSamples();

    // now that we are wrapped and valid, assign to mReadPos
    mReadPos = newReadPos;
}

