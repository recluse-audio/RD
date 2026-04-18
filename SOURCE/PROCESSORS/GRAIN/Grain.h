/**
 * Grain.h
 * Created by Ryan Devens
 *
 * Represents a single grain for TD-PSOLA (Time-Domain Pitch Synchronous Overlap-Add) processing.
 * A Grain combines a SynthMark (which defines the pitch source range and synth destination range)
 * with a current index that tracks consumption progress as samples are processed.
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PITCH/SynthMark.h"
#include "Window.h"
#include "CircularBuffer.h"

/**
 * Grain represents an active grain being synthesized in TD-PSOLA processing.
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
class Grain
{
public:
    /**
     * Construct a grain from a SynthMark, Window reference, source buffer, and lookahead size.
     * @param mark The SynthMark containing pitch source and synth destination data
     * @param window Reference to the Window for applying windowing functions
     * @param sourceBuffer Reference to the CircularBuffer containing source audio
     * @param lookaheadSize The lookahead size (in samples) to offset the write range
     */
    Grain(const SynthMark& mark, Window& window, CircularBuffer& sourceBuffer, juce::int64 lookaheadSize)
        : synthMark(mark)
        , window(window)
        , sourceBuffer(sourceBuffer)
        , writeRangeStart(mark.synthRangeStart + lookaheadSize)
        , writeRangeEnd(mark.synthRangeEnd + lookaheadSize)
        , currentIndex(0)
    {
    }

    bool isValid() const    { return synthMark.isValid(); }

    bool isFinished() const { return currentIndex >= synthMark.getSynthRangeLength(); }

    float getProgress() const
    {
        auto length = synthMark.getSynthRangeLength();
        return length > 0 ? static_cast<float>(currentIndex) / static_cast<float>(length) : 0.0f;
    }

    void consume(juce::int64 numSamples) { currentIndex += numSamples; }

    void reset() { currentIndex = 0; }

    /**
     * Calculate the overlap between this grain's write range and a process block.
     * Returns an empty range if there is no overlap.
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
     * Reads from source buffer, applies Hann/Tukey window, and overlap-adds into output.
     */
    void process(juce::AudioBuffer<float>& outputBuffer, juce::int64 blockStartSample, juce::int64 blockEndSample)
    {
        if (!isValid() || isFinished())
            return;

        auto overlap = getOverlapWithBlock(blockStartSample, blockEndSample);

        if (overlap.isEmpty())
            return;

        int grainPeriod = static_cast<int>(synthMark.getSynthRangeLength());

        for (juce::int64 absoluteSample = overlap.getStart(); absoluteSample < overlap.getEnd(); ++absoluteSample)
        {
            juce::int64 grainIndex    = absoluteSample - writeRangeStart;
            juce::int64 pitchPosition = synthMark.pitchRangeStart + grainIndex;
            int outputIndex           = static_cast<int>(absoluteSample - blockStartSample);

            // Voiced grains apply the shaped window; unvoiced use rectangular (pass-through)
            float windowValue = synthMark.isVoiced
                ? window.getValueAtIndexInPeriod(static_cast<int>(grainIndex), grainPeriod)
                : 1.0f;

            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            {
                float sourceSample = sourceBuffer.getSample(ch, pitchPosition);
                outputBuffer.addSample(ch, outputIndex, sourceSample * windowValue);
            }
        }

        consume(overlap.getLength());
    }

    /**
     * Update the grain with new SynthMark data (for grain pool reuse).
     * The window reference remains unchanged (all grains share the same window).
     */
    void setGrain(const SynthMark& mark, juce::int64 lookaheadSize)
    {
        synthMark       = mark;
        writeRangeStart = mark.synthRangeStart + lookaheadSize;
        writeRangeEnd   = mark.synthRangeEnd   + lookaheadSize;
        currentIndex    = 0;
    }

    juce::int64 getCurrentPitchPosition()  const { return synthMark.pitchRangeStart + currentIndex; }
    juce::int64 getCurrentSynthPosition()  const { return synthMark.synthRangeStart + currentIndex; }
    juce::int64 getCurrentWritePosition()  const { return writeRangeStart + currentIndex; }

    juce::int64 getSamplesRemaining() const
    {
        auto remaining = synthMark.getSynthRangeLength() - currentIndex;
        return remaining > 0 ? remaining : 0;
    }

    const SynthMark& getSynthMark() const { return synthMark; }

    juce::Range<juce::int64> getWriteRange() const { return juce::Range<juce::int64>(writeRangeStart, writeRangeEnd + 1); }

    juce::int64 getWriteRangeStart()  const { return writeRangeStart; }
    juce::int64 getWriteRangeEnd()    const { return writeRangeEnd; }
    juce::int64 getWriteRangeLength() const { return writeRangeEnd - writeRangeStart + 1; }
    juce::int64 getCurrentIndex()     const { return currentIndex; }

    Window&       getWindow()       { return window; }
    const Window& getWindow() const { return window; }

private:
    SynthMark      synthMark;
    Window&        window;
    CircularBuffer& sourceBuffer;
    juce::int64    writeRangeStart;
    juce::int64    writeRangeEnd;
    juce::int64    currentIndex;
};
