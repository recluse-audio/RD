/**
 * TD_Grain.h
 * Created by Ryan Devens
 *
 * Represents a single grain for TD-PSOLA (Time-Domain Pitch Synchronous Overlap-Add) processing.
 * A TD_Grain combines a SynthMark (which defines the pitch source range and synth destination range)
 * with a current index that tracks consumption progress as samples are processed.
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PITCH/SynthMark.h"
#include "Window.h"
#include "CircularBuffer.h"

/**
 * TD_Grain represents an active grain being synthesized in TD-PSOLA processing.
 *
 * Each grain:
 * - References a SynthMark (contains pitch mark source data and synth mark destination data)
 * - Owns a reference to a Window for applying windowing functions
 * - Maintains a current index that increments as samples are consumed
 * - Has a write range that is the synth range offset by the lookahead size
 * - Provides methods to track consumption progress and calculate read/write positions
 *
 * The write range accounts for the lookahead delay needed for pitch detection.
 * For example: synth range 0-2047 with lookahead 2048 → write range 2048-4095
 *
 * The grain is considered "finished" when the current index reaches the synth range length.
 */
class TD_Grain
{
public:
    /**
     * Construct a grain from a SynthMark, Window reference, source buffer, and lookahead size.
     * @param mark The SynthMark containing pitch source and synth destination data
     * @param window Reference to the Window for applying windowing functions
     * @param sourceBuffer Reference to the CircularBuffer containing source audio
     * @param lookaheadSize The lookahead size (in samples) to offset the write range
     */
    TD_Grain(const SynthMark& mark, Window& window, CircularBuffer& sourceBuffer, juce::int64 lookaheadSize)
        : synthMark(mark)
        , window(window)
        , sourceBuffer(sourceBuffer)
        , writeRangeStart(mark.synthRangeStart + lookaheadSize)
        , writeRangeEnd(mark.synthRangeEnd + lookaheadSize)
        , currentIndex(0)
    {
    }

    /**
     * Check if this grain is valid.
     * @return True if the associated SynthMark is valid
     */
    bool isValid() const
    {
        return synthMark.isValid();
    }

    /**
     * Check if this grain has finished processing.
     * A grain is finished when the current index has reached or exceeded the synth range length.
     * @return True if all samples have been consumed
     */
    bool isFinished() const
    {
        return currentIndex >= synthMark.getSynthRangeLength();
    }

    /**
     * Get the current consumption progress as a ratio.
     * @return Progress from 0.0 (just started) to 1.0 (finished)
     */
    float getProgress() const
    {
        auto length = synthMark.getSynthRangeLength();
        return length > 0 ? static_cast<float>(currentIndex) / static_cast<float>(length) : 0.0f;
    }

    /**
     * Increment the current index by the given number of samples.
     * @param numSamples Number of samples consumed
     */
    void consume(juce::int64 numSamples)
    {
        currentIndex += numSamples;
    }

    /**
     * Reset the grain to start from the beginning.
     */
    void reset()
    {
        currentIndex = 0;
    }

    /**
     * Calculate the overlap between this grain's write range and a process block.
     * Returns an empty range if there is no overlap.
     *
     * @param blockStartSample Absolute sample time where the block starts (inclusive)
     * @param blockEndSample Absolute sample time where the block ends (exclusive)
     * @return Absolute sample range of the overlap, empty if none
     */
    juce::Range<juce::int64> getOverlapWithBlock(juce::int64 blockStartSample, juce::int64 blockEndSample) const
    {
        juce::int64 overlapStart = juce::jmax(writeRangeStart, blockStartSample);
        juce::int64 overlapEnd   = juce::jmin(writeRangeEnd + 1, blockEndSample); // writeRangeEnd is inclusive

        if (overlapStart >= overlapEnd)
            return juce::Range<juce::int64>();

        return juce::Range<juce::int64>(overlapStart, overlapEnd);
    }

    /**
     * Process this grain for the given output block.
     * Calculates overlap between grain's write range and block's time range,
     * then reads from source buffer, applies windowing, and writes to output.
     *
     * @param outputBuffer Output buffer to write to (samples are added for overlap-add)
     * @param blockStartSample Absolute sample time where the output block starts
     * @param blockEndSample Absolute sample time where the output block ends (exclusive)
     */
    void process(juce::AudioBuffer<float>& outputBuffer, juce::int64 blockStartSample, juce::int64 blockEndSample)
    {
        // Skip if grain is not valid or already finished
        if (!isValid() || isFinished())
            return;

        auto overlap = getOverlapWithBlock(blockStartSample, blockEndSample);

        if (overlap.isEmpty())
            return;

        // Get the grain's period (synth range length) for window calculation
        int grainPeriod = static_cast<int>(synthMark.getSynthRangeLength());

        // Process each sample in the overlap
        for (juce::int64 absoluteSample = overlap.getStart(); absoluteSample < overlap.getEnd(); ++absoluteSample)
        {
            // Calculate index within the grain
            juce::int64 grainIndex = absoluteSample - writeRangeStart;

            // Calculate source position to read from (pitch range)
            juce::int64 pitchPosition = synthMark.pitchRangeStart + grainIndex;

            // Calculate output buffer index
            int outputIndex = static_cast<int>(absoluteSample - blockStartSample);

            // Voiced grains apply the shaped window; unvoiced use rectangular (pass-through)
            float windowValue = synthMark.isVoiced
                ? window.getValueAtIndexInPeriod(static_cast<int>(grainIndex), grainPeriod)
                : 1.0f;

            // Process each channel
            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            {
                // Read source sample from circular buffer
                float sourceSample = sourceBuffer.getSample(ch, pitchPosition);

                // Apply window and add to output (overlap-add synthesis)
                outputBuffer.addSample(ch, outputIndex, sourceSample * windowValue);
            }
        }

        consume(overlap.getLength());
    }

    /**
     * Update the grain with new SynthMark data (for grain pool reuse).
     * Allows reassigning the grain without reallocating.
     * The window reference remains unchanged (all grains share the same window).
     * @param mark New SynthMark data
     * @param lookaheadSize New lookahead size
     */
    void setGrain(const SynthMark& mark, juce::int64 lookaheadSize)
    {
        // Copy the new synth mark
        synthMark = mark;

        // Recalculate write range
        writeRangeStart = mark.synthRangeStart + lookaheadSize;
        writeRangeEnd = mark.synthRangeEnd + lookaheadSize;

        currentIndex = 0;
    }

    /**
     * Get the current read position in the pitch (source) buffer.
     * This is the absolute position in the circular buffer to read from.
     * @return Absolute sample position in source buffer
     */
    juce::int64 getCurrentPitchPosition() const
    {
        return synthMark.pitchRangeStart + currentIndex;
    }

    /**
     * Get the current synth position (without lookahead offset).
     * This is the theoretical synth position from the SynthMark.
     * @return Absolute sample position in synth range
     */
    juce::int64 getCurrentSynthPosition() const
    {
        return synthMark.synthRangeStart + currentIndex;
    }

    /**
     * Get the current write position in the output buffer (with lookahead offset).
     * This is the actual absolute position where samples should be written.
     * @return Absolute sample position in output buffer (synth position + lookahead)
     */
    juce::int64 getCurrentWritePosition() const
    {
        return writeRangeStart + currentIndex;
    }

    /**
     * Get the number of samples remaining to be consumed.
     * @return Samples remaining (0 if finished)
     */
    juce::int64 getSamplesRemaining() const
    {
        auto length = synthMark.getSynthRangeLength();
        auto remaining = length - currentIndex;
        return remaining > 0 ? remaining : 0;
    }

    /**
     * Get the underlying SynthMark.
     * @return Reference to the SynthMark
     */
    const SynthMark& getSynthMark() const
    {
        return synthMark;
    }

    /**
     * Get the write range as a juce::Range (synth range offset by lookahead).
     * @return Write range object
     */
    juce::Range<juce::int64> getWriteRange() const
    {
        return juce::Range<juce::int64>(writeRangeStart, writeRangeEnd + 1);
    }

    /**
     * Get the write range start position.
     * @return Absolute sample position where writing begins
     */
    juce::int64 getWriteRangeStart() const
    {
        return writeRangeStart;
    }

    /**
     * Get the write range end position.
     * @return Absolute sample position where writing ends
     */
    juce::int64 getWriteRangeEnd() const
    {
        return writeRangeEnd;
    }

    /**
     * Get the length of the write range.
     * @return Write range length in samples
     */
    juce::int64 getWriteRangeLength() const
    {
        return writeRangeEnd - writeRangeStart + 1;
    }

    /**
     * Get the current index.
     * @return Current consumption index in samples
     */
    juce::int64 getCurrentIndex() const
    {
        return currentIndex;
    }

    /**
     * Get the window reference.
     * @return Reference to the Window
     */
    Window& getWindow()
    {
        return window;
    }

    /**
     * Get the window reference (const version).
     * @return Const reference to the Window
     */
    const Window& getWindow() const
    {
        return window;
    }

private:
    SynthMark synthMark;            // The synthesis mark (pitch source + synth destination)
    Window& window;                 // Reference to the window for applying windowing functions
    CircularBuffer& sourceBuffer;   // Reference to the circular buffer containing source audio
    juce::int64 writeRangeStart;    // Write range start (synth range start + lookahead)
    juce::int64 writeRangeEnd;      // Write range end (synth range end + lookahead)
    juce::int64 currentIndex;       // Current consumption index (increments as samples are processed)
};
