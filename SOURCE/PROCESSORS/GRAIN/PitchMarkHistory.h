/**
 * PitchMarkHistory.h
 *
 * REALTIME-SAFE pitch mark history storage using a ring buffer.
 *
 * PURPOSE:
 * Stores long-term history of pitch marks for analysis, debugging, and algorithm development.
 * Complements PitchMarkBuffer (current block only) with persistent cross-block storage.
 *
 * WHEN TO USE:
 * - To analyze pitch tracking stability over time
 * - For debugging pitch mark placement and jitter
 * - To calculate period variance and detect vibrato
 * - To access recent marks for improved mark prediction
 * - For visualizing pitch tracking behavior
 *
 * SYNCHRONIZATION WITH CIRCULAR BUFFER:
 * Marks are stored as ABSOLUTE sample positions (e.g., sample 10000, 10100, 10200...).
 * When reading audio at these positions, CircularBuffer::getWrappedIndex() converts them
 * to buffer indices via modulo operation. This works correctly because:
 *   - CircularBuffer is sized for your use case (~4096 samples at 44.1kHz)
 *   - Marks are typically used within that time window
 *   - Marks older than circularBufferSize wrap to newer audio positions (by design)
 *
 * LIFETIME:
 * - Allocated ONCE in prepareToPlay() (one-time allocation, not on audio thread)
 * - Persists across processBlock() calls
 * - Oldest marks automatically overwritten when buffer is full (ring behavior)
 * - Cleared when audio playback stops or sample rate changes
 *
 * REALTIME SAFETY:
 * - Pre-allocated in prepareToPlay() (NOT on audio thread)
 * - addMark() only writes to pre-allocated buffer (no allocation)
 * - Ring buffer wraps automatically, no dynamic growth
 * - Safe to call on audio thread after prepareToPlay()
 *
 * EXAMPLE USAGE:
 * void prepareToPlay(double sampleRate, int samplesPerBlock)
 * {
 *     mAnalysisMarkHistory.prepareToPlay(sampleRate, samplesPerBlock);
 * }
 *
 * void processBlock(AudioBuffer<float>& buffer, MidiBuffer&)
 * {
 *     // Find and store pitch mark
 *     int64 mark = findPitchMark();
 *     mAnalysisMarkHistory.addMark(mark);
 *
 *     // Later: analyze recent marks
 *     if (mAnalysisMarkHistory.getCount() >= 2)
 *     {
 *         int64 mark0 = mAnalysisMarkHistory.getMark(0);  // Most recent
 *         int64 mark1 = mAnalysisMarkHistory.getMark(1);  // Previous
 *         float period = static_cast<float>(mark0 - mark1);
 *     }
 * }
 */

#pragma once
#include "Util/Juce_Header.h"
#include <vector>

class PitchMarkHistory
{
public:
    // Capacity: 512 marks
    // At 100Hz (period ~441 samples at 44.1kHz), this covers ~5 seconds of history
    // At 400Hz (period ~110 samples), this covers ~1.25 seconds
    static constexpr int HISTORY_CAPACITY = 512;

    // Allocate ring buffer (call from prepareToPlay, NOT on audio thread)
    void prepareToPlay(double sampleRate, int maxSamplesPerBlock)
    {
        mMarkRingBuffer.resize(HISTORY_CAPACITY);  // ONE-TIME allocation
        clear();
    }

    // Add pitch mark to history (realtime-safe, no allocation)
    // Marks are stored as absolute sample positions
    // Oldest mark is automatically overwritten when buffer is full
    void addMark(juce::int64 absoluteSamplePosition)
    {
        mMarkRingBuffer[mWritePos] = absoluteSamplePosition;
        mWritePos = (mWritePos + 1) % HISTORY_CAPACITY;

        if (mCount < HISTORY_CAPACITY)
        {
            mCount++;
        }
    }

    // Get mark by index from newest (0 = most recent, 1 = previous, etc.)
    // Returns absolute sample position
    // Assertion checks bounds in debug builds
    juce::int64 getMark(int indexFromNewest) const
    {
        jassert(indexFromNewest < mCount);
        int pos = (mWritePos - 1 - indexFromNewest + HISTORY_CAPACITY) % HISTORY_CAPACITY;
        return mMarkRingBuffer[pos];
    }

    // Get number of marks currently stored (0 to HISTORY_CAPACITY)
    int getCount() const
    {
        return mCount;
    }

    // Clear all marks (resets counters only, no deallocation)
    void clear()
    {
        mWritePos = 0;
        mCount = 0;
    }

private:
    std::vector<juce::int64> mMarkRingBuffer;  // Allocated ONCE in prepareToPlay
    int mWritePos = 0;
    int mCount = 0;
};
