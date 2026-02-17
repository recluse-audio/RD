/**
 * TD_Granulator.h
 * Created by Ryan Devens
 *
 * Manages TD-PSOLA grain synthesis.
 * Owns an array of TD_Grain objects and a Window for windowing operations.
 * Creates grains from SynthMark arrays for overlap-add synthesis.
 */

#pragma once
#include "Util/Juce_Header.h"
#include "TD_Grain.h"
#include "PITCH/SynthMark.h"
#include "Window.h"
#include "CircularBuffer.h"
#include <vector>

/**
 * TD_Granulator manages grain-based synthesis for TD-PSOLA.
 *
 * Uses a voice pool pattern where grains are pre-allocated and reused:
 * - Pre-allocates a fixed pool of TD_Grain objects in prepare()
 * - All grains share a single Window reference
 * - generateGrains() finds finished/invalid grains and reuses them
 * - Grains "free themselves" by becoming finished (like synth voices)
 * - No allocations occur during grain generation (real-time safe)
 *
 * Key responsibilities:
 * - Owns a pre-allocated pool of TD_Grain objects
 * - Owns a Window for applying windowing functions to grains
 * - Reuses finished grains for new SynthMarks
 * - Manages lookahead offset for proper temporal alignment
 *
 * Usage:
 * 1. prepare() to pre-allocate grain pool with max size
 * 2. generateGrains() to assign new SynthMarks to available grains
 * 3. Access grains via getGrains() for synthesis processing
 */
class TD_Granulator
{
public:
    /**
     * Construct the granulator with a reference to the source circular buffer.
     * @param sourceBuffer Reference to the circular buffer containing pitch-marked source audio
     */
    explicit TD_Granulator(CircularBuffer& sourceBuffer);
    ~TD_Granulator();

    /**
     * Prepare the granulator.
     * Allocates capacity for the grains array upfront to avoid real-time allocations.
     *
     * @param sampleRate Current sample rate
     * @param numChannels Number of audio channels
     * @param lookaheadSamples Lookahead size (in samples) for write range offset
     * @param maxGrains Maximum number of grains to allocate capacity for
     */
    void prepare(double sampleRate, int numChannels, juce::int64 lookaheadSamples, int maxGrains);

    /**
     * Generate grains from an array of SynthMarks.
     * Creates one TD_Grain for each SynthMark, each referencing the shared Window.
     * Does not allocate memory if the number of synth marks is within the pre-allocated capacity.
     *
     * @param synthMarks Array of synth marks to create grains from
     * @throws Asserts if synthMarks.size() exceeds the maximum capacity set in prepare()
     */
    void generateGrains(const std::vector<SynthMark>& synthMarks);

    /**
     * Process active grains and synthesize audio using overlap-add.
     * Each grain checks if its write range overlaps with the output block's time range.
     * If overlapping, the grain reads from the source buffer, applies windowing, and writes to output.
     *
     * @param outputBuffer Output buffer to write synthesized audio to (samples are added for overlap-add)
     * @param blockStartSample Absolute sample time where this output block starts
     * @param blockEndSample Absolute sample time where this output block ends (exclusive)
     */
    void process(juce::AudioBuffer<float>& outputBuffer,
                 juce::int64 blockStartSample,
                 juce::int64 blockEndSample);

    /**
     * Get the grains array.
     * @return Reference to the grains vector
     */
    std::vector<TD_Grain>& getGrains() { return mGrains; }

    /**
     * Get the grains array (const version).
     * @return Const reference to the grains vector
     */
    const std::vector<TD_Grain>& getGrains() const { return mGrains; }

    /**
     * Get the number of grains.
     * @return Number of grains in the array
     */
    int getNumGrains() const { return static_cast<int>(mGrains.size()); }

    /**
     * Get the window reference.
     * @return Reference to the Window
     */
    Window& getWindow() { return mWindow; }

    /**
     * Get the window reference (const version).
     * @return Const reference to the Window
     */
    const Window& getWindow() const { return mWindow; }

    /**
     * Get the lookahead size.
     * @return Lookahead size in samples
     */
    juce::int64 getLookaheadSize() const { return mLookaheadSamples; }

    /**
     * Reset all grains to invalid state (makes them available for reuse).
     * Does not deallocate the grain pool - maintains pre-allocated capacity.
     */
    void reset();

private:
    CircularBuffer& mSourceBuffer;      // Reference to source circular buffer (pitch-marked audio)
    std::vector<TD_Grain> mGrains;      // Array of grains (size matches synth marks)
    Window mWindow;                      // Window for grain windowing functions
    juce::int64 mLookaheadSamples = 0;  // Lookahead size for write range offset
    int mMaxGrains = 0;                  // Maximum number of grains (capacity pre-allocated)
    double mSampleRate = 44100.0;        // Current sample rate
    int mNumChannels = 2;                // Number of audio channels
};
