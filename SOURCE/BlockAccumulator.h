/**
 * Created by Ryan Devens on 2026-02-18
 */

#pragma once
#include "Util/Juce_Header.h"

/**
 * Accumulates incoming audio blocks into a fixed-size target block using a double-buffer scheme.
 *
 * Incoming blocks (typically smaller than the target size) are written into the active buffer.
 * When the active buffer fills, blockReady() returns true. If an incoming push() would overflow
 * the active buffer, the remainder is written into the start of the queued buffer so no samples
 * are lost. Call popReadyBlock() after consuming the ready block to promote the queued buffer.
 */
class BlockAccumulator
{
public:
    BlockAccumulator();
    ~BlockAccumulator();

    // @param numChannels    Number of audio channels
    // @param targetBlockSize Desired accumulated block size in samples
    void setSize(int numChannels, int targetBlockSize);
    void reset();

    // Writes incoming audio into the accumulator.
    // If the block overflows the active buffer, the remainder is written into the queued buffer.
    void push(const juce::AudioBuffer<float>& inputBuffer);

    // Advances the sample count by numSamples without copying any audio data.
    // Use this when the audio itself lives elsewhere (e.g. a CircularBuffer) and only
    // the timing gate is needed.
    void incrementSamples(int numSamples);

    // Returns true when the active buffer has been fully accumulated.
    bool blockReady() const;

    // Returns a reference to the fully accumulated buffer.
    // Only valid when blockReady() is true.
    const juce::AudioBuffer<float>& getReadyBlock() const;

    // Consumes the ready block and promotes the queued buffer to active.
    // Resets the blockReady flag. Call this after you are finished processing the ready block.
    void popReadyBlock();

    int getTargetBlockSize() const { return mTargetBlockSize; }
    int getNumChannels() const { return mNumChannels; }

private:
    friend class BlockAccumulatorTest;

    juce::AudioBuffer<float> mBuffers[2];
    int mActiveIdx = 0;
    int mWritePos = 0;        // write cursor in the active buffer
    int mQueuedWritePos = 0;  // how much overflow has been written into the queued buffer
    bool mBlockReady = false;
    int mTargetBlockSize = 2048;
    int mNumChannels = 2;

    // Copies numSamples from source (starting at sourceOffset) into mBuffers[bufferIdx]
    // (starting at destOffset). Handles channel count mismatches by clamping.
    void _writeToBuffer(int bufferIdx, const juce::AudioBuffer<float>& source,
                        int sourceOffset, int destOffset, int numSamples);
};
