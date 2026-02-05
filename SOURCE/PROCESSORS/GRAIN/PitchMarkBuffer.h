/**
 * PitchMarkBuffer.h
 *
 * REALTIME-SAFE pitch mark storage for the current processing block.
 *
 * PURPOSE:
 * Stores pitch marks (peak positions) found during the current processBlock() call.
 * Marks are stored as absolute sample positions relative to mSamplesProcessed.
 *
 * WHEN TO USE:
 * - During pitch detection phase in processBlock()
 * - To track multiple peaks found within a single buffer
 * - For debugging pitch tracking behavior
 * - As input to synthesis algorithms that need mark positions
 *
 * LIFETIME:
 * - Cleared at the START of each processBlock()
 * - Populated during pitch detection
 * - Valid only for the duration of current processBlock()
 * - NOT persistent across blocks (use PitchMarkHistory for that)
 *
 * REALTIME SAFETY:
 * - Zero allocations: uses fixed-size std::array
 * - clear() only resets counter (no memory operations)
 * - addMark() is bounds-checked with assertion
 * - Safe to call on audio thread
 *
 * EXAMPLE USAGE:
 * void processBlock(AudioBuffer<float>& buffer, MidiBuffer&)
 * {
 *     mCurrentBlockMarks.clear();  // Start fresh each block
 *
 *     // Find peaks during detection
 *     for (int i = 0; i < detectionWindow; ++i)
 *     {
 *         if (isPeak(i))
 *         {
 *             int64 absolutePos = mSamplesProcessed + i;
 *             mCurrentBlockMarks.addMark(absolutePos);
 *         }
 *     }
 *
 *     // Use marks for synthesis
 *     for (int i = 0; i < mCurrentBlockMarks.getCount(); ++i)
 *     {
 *         int64 markPos = mCurrentBlockMarks[i];
 *         synthesizeGrainAt(markPos);
 *     }
 * }
 */

#pragma once
#include "Util/Juce_Header.h"
#include <array>

class PitchMarkBuffer
{
public:
    // Maximum marks per buffer: ~16 covers up to 50Hz at 512 sample blocks
    // (50Hz period = ~882 samples at 44.1kHz, so 512/882 < 1 mark expected)
    static constexpr int MAX_MARKS_PER_BUFFER = 16;

    PitchMarkBuffer()
    {
        clear();
    }

    // Clear all marks (resets counter only, no allocation)
    // Call at start of each processBlock()
    void clear()
    {
        mCount = 0;
    }

    // Add a pitch mark at absolute sample position
    // Position should be relative to total samples processed since start
    // Silently ignores if buffer is full (assertion in debug builds)
    void addMark(juce::int64 absolutePosition)
    {
        jassert(mCount < MAX_MARKS_PER_BUFFER);
        if (mCount < MAX_MARKS_PER_BUFFER)
        {
            mPositions[mCount++] = absolutePosition;
        }
    }

    // Get number of marks currently stored
    int getCount() const
    {
        return mCount;
    }

    // Access mark by index (0 = first mark added, getCount()-1 = last)
    // Assertion checks bounds in debug builds
    juce::int64 operator[](int idx) const
    {
        jassert(idx < mCount);
        return mPositions[idx];
    }

private:
    std::array<juce::int64, MAX_MARKS_PER_BUFFER> mPositions;
    int mCount = 0;
};
