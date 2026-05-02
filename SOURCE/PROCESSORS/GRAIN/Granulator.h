/**
 * Granulator.h
 * Created by Ryan Devens
 *
 * Manages TD-PSOLA grain synthesis.
 * Owns an array of Grain objects and a Window for windowing operations.
 * Creates grains from SynthMark arrays for overlap-add synthesis.
 */

#pragma once
#include "Util/Juce_Header.h"
#include "Grain.h"
#include "PITCH/SynthMark.h"
#include "Window.h"
#include "CircularBuffer.h"
#include "DATA_LOGGER/DataLogger.h"
#include <vector>
#include <unordered_map>

/**
 * Granulator manages grain-based synthesis for TD-PSOLA pitch shifting.
 *
 * Uses a voice pool pattern where grains are pre-allocated and reused:
 * - Pre-allocates a fixed pool of Grain objects in prepare()
 * - All grains share a single Window reference
 * - generateGrains() finds finished/invalid grains and reuses them
 * - Grains "free themselves" by becoming finished (like synth voices)
 * - No allocations occur during grain generation (real-time safe)
 *
 * Usage:
 * 1. prepare() to pre-allocate grain pool with max size
 * 2. generateGrains() to assign new SynthMarks to available grains
 * 3. Access grains via getGrains() for synthesis processing
 */
class Granulator : public DataLogger
{
public:
    explicit Granulator(CircularBuffer& sourceBuffer);
    ~Granulator();

    /** DataLogger override. Appends per-grain rows to synthesis_grains.csv
     *  when a generateGrains() call flagged a pending log; no-op otherwise
     *  (so parent cascade invocations don't double-log). */
    bool doLogData() override;

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
     * Does not allocate memory if the number of synth marks is within the pre-allocated capacity.
     */
    void generateGrains(const std::vector<SynthMark>& synthMarks);

    /**
     * Process active grains and synthesize audio using overlap-add.
     */
    void process(juce::AudioBuffer<float>& outputBuffer,
                 juce::int64 blockStartSample,
                 juce::int64 blockEndSample);

    std::vector<Grain>&       getGrains()       { return mGrains; }
    const std::vector<Grain>& getGrains() const { return mGrains; }
    int getNumGrains() const { return static_cast<int>(mGrains.size()); }

    Window&       getWindow()       { return mWindow; }
    const Window& getWindow() const { return mWindow; }

    juce::int64 getLookaheadSize() const { return mLookaheadSamples; }

    /**
     * Reset all grains to invalid state (makes them available for reuse).
     * Does not deallocate the grain pool - maintains pre-allocated capacity.
     */
    void reset();

private:
    CircularBuffer&    mSourceBuffer;
    std::vector<Grain> mGrains;
    Window             mWindow;
    juce::int64        mLookaheadSamples = 0;
    int                mMaxGrains        = 0;
    double             mSampleRate       = 44100.0;
    int                mNumChannels      = 2;

    // Snapshot of last generateGrains() call for per-grain logging.
    std::vector<SynthMark> mLastSynthMarks;
    bool                   mGenerateLogPending = false;

    // Running counters / id-assignment for synthesis_grains.csv.
    int mNextGrainId    = 0;
    int mNextAnalysisId = 0;
    std::unordered_map<juce::int64, int> mAnalysisIdByPitchCenter;
    float mWindowAlpha = 0.8f; // Tukey alpha currently configured.
};
