/**
 * Created by Ryan Devens on 2026-02-18
 */

#include "BlockAccumulator.h"

BlockAccumulator::BlockAccumulator() {}

BlockAccumulator::~BlockAccumulator() {}

void BlockAccumulator::setSize(int numChannels, int targetBlockSize)
{
    mNumChannels = numChannels;
    mTargetBlockSize = targetBlockSize;

    for (auto& buffer : mBuffers)
    {
        buffer.setSize(numChannels, targetBlockSize);
        buffer.clear();
    }

    reset();
}

void BlockAccumulator::reset()
{
    mActiveIdx = 0;
    mWritePos = 0;
    mQueuedWritePos = 0;
    mBlockReady = false;
}

void BlockAccumulator::push(const juce::AudioBuffer<float>& inputBuffer)
{
    const int incomingSize = inputBuffer.getNumSamples();
    const int remaining = mTargetBlockSize - mWritePos;

    if (incomingSize <= remaining)
    {
        _writeToBuffer(mActiveIdx, inputBuffer, 0, mWritePos, incomingSize);
        mWritePos += incomingSize;

        if (mWritePos == mTargetBlockSize)
            mBlockReady = true;
    }
    else
    {
        // Fill the rest of the active buffer, then spill overflow into the queued buffer.
        _writeToBuffer(mActiveIdx, inputBuffer, 0, mWritePos, remaining);
        mBlockReady = true;

        const int overflow = incomingSize - remaining;
        const int queuedIdx = 1 - mActiveIdx;
        _writeToBuffer(queuedIdx, inputBuffer, remaining, mQueuedWritePos, overflow);
        mQueuedWritePos += overflow;
    }
}

void BlockAccumulator::incrementSamples(int numSamples)
{
    const int remaining = mTargetBlockSize - mWritePos;

    if (numSamples <= remaining)
    {
        mWritePos += numSamples;

        if (mWritePos == mTargetBlockSize)
            mBlockReady = true;
    }
    else
    {
        mBlockReady = true;
        mQueuedWritePos += (numSamples - remaining);
    }
}

bool BlockAccumulator::blockReady() const
{
    return mBlockReady;
}

const juce::AudioBuffer<float>& BlockAccumulator::getReadyBlock() const
{
    return mBuffers[mActiveIdx];
}

void BlockAccumulator::popReadyBlock()
{
    jassert(mBlockReady);

    mActiveIdx = 1 - mActiveIdx;
    mWritePos = mQueuedWritePos;
    mQueuedWritePos = 0;
    mBlockReady = false;

    // Edge case: if the newly promoted buffer is already full, mark it ready immediately.
    if (mWritePos == mTargetBlockSize)
        mBlockReady = true;
}

void BlockAccumulator::_writeToBuffer(int bufferIdx, const juce::AudioBuffer<float>& source,
                                      int sourceOffset, int destOffset, int numSamples)
{
    if (numSamples <= 0)
        return;

    auto& dest = mBuffers[bufferIdx];

    for (int ch = 0; ch < dest.getNumChannels(); ++ch)
    {
        const int srcCh = juce::jmin(ch, source.getNumChannels() - 1);
        dest.copyFrom(ch, destOffset, source, srcCh, sourceOffset, numSamples);
    }
}
