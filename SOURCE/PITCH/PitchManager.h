/**
 * PitchManager.h
 * Created by Ryan Devens
 *
 * Manages pitch detection and pitch mark tracking.
 * Coordinates FFT_PitchDetector and PitchMarker, and stores pitch marks in a FIFO buffer.
 */

#pragma once
#include <vector>

#include "Util/Juce_Header.h"
#include "FFT_PitchDetector.h"
#include "../CircularBuffer.h"
#include "PitchMarker.h"
#include "SynthMarker.h"


namespace PitchManagerConstants
{
    static constexpr int kDefaultDetectionWindowSize = 2048;
    static constexpr double kPitchMarkBufferSeconds = 30.0;  // 30 seconds of pitch marks
}

/**
 * PitchManager coordinates pitch detection and pitch mark tracking.
 *
 * Responsibilities:
 * - Runs pitch detection on a window of samples read from a CircularBuffer
 * - Tracks pitch marks using PitchMarker
 * - Stores pitch marks in a circular FIFO buffer
 *
 * Usage:
 * 1. Call prepare() with sample rate
 * 2. Call detect() with the circular buffer and an absolute start index
 * 3. Query getCurrentPeriod() to get detected period
 * 4. Use getPitchMarker().doPitchMarking() to locate pitch marks in the circular buffer
 */
class PitchManager
{
public:
    PitchManager();
    ~PitchManager();

    /**
     * Prepare the pitch manager.
     * @param sampleRate Sample rate
     * @param detectionWindowSize Size of detection window in samples (default 2048)
     */
    void prepare(double sampleRate, int numChannels = 2, int detectionWindowSize = PitchManagerConstants::kDefaultDetectionWindowSize);

    /**
     * Run pitch detection on a window of samples read from the circular buffer.
     * The window size is determined by the current detectionWindowSize.
     *
     * @param circularBuffer Circular buffer containing the audio to analyze
     * @param startAbsIndex Absolute sample index to start reading from
     * @return Detected period in samples, or -1 if no pitch detected
     */
    float detect(CircularBuffer& circularBuffer, juce::int64 startAbsIndex, float shiftRatio = 1.0f);

    /**
     * Get access to the pitch marks FIFO from the PitchMarker.
     * @return Reference to the pitch marks vector
     */
    const std::vector<PitchMark>& getPitchMarks() const { return mPitchMarker->getPitchMarks(); }

    /**
     * Get the last stored PitchMark.
     * @return Last pitch mark, or invalid PitchMark if none stored
     */
    PitchMark getLastPitchMark() const { return mPitchMarker->getLastPitchMark(); }

    /**
     * Get pitch marks within a given absolute time range.
     * Returns all pitch marks whose center position falls within the query range.
     *
     * @param range Absolute time range to query
     * @return Vector of pitch marks whose center is in the range
     */
    std::vector<PitchMark> getPitchMarksInRange(juce::Range<juce::int64> range) const { return mPitchMarker->getPitchMarksInRange(range); }

    /**
     * Get synth marks within a given absolute time range.
     * Returns all synth marks whose center position falls within the query range.
     *
     * @param range Absolute time range to query (in synth/output time)
     * @return Vector of synth marks whose center is in the range
     */
    std::vector<SynthMark> getSynthMarksInRange(juce::Range<juce::int64> range) const { return mSynthMarker->getSynthMarksInRange(range); }

    /**
     * Get access to the synth marks array from the SynthMarker.
     * @return Reference to the synth marks vector
     */
    const std::vector<SynthMark>& getSynthMarks() const { return mSynthMarker->getSynthMarks(); }

    /**
     * Get the current detected period.
     * @return Period in samples, or -1 if no pitch detected
     */
    float getCurrentPeriod() const { return mCurrentPeriod; }

    /**
     * Reset all state.
     */
    void reset();

    /**
     * Get access to the pitch detector for configuration.
     */
    FFT_PitchDetector& getPitchDetector() { return mPitchDetector; }

    /**
     * Get access to the pitch marker for configuration.
     */
    PitchMarker& getPitchMarker() { return *mPitchMarker; }

    /**
     * Get access to the synth marker for configuration.
     */
    SynthMarker& getSynthMarker() { return *mSynthMarker; }

private:
    // Pitch detection and marking
    FFT_PitchDetector mPitchDetector;
    std::unique_ptr<PitchMarker> mPitchMarker;
    std::unique_ptr<SynthMarker> mSynthMarker;

    // Scratch buffer for extracting a detection window from the circular buffer
    juce::AudioBuffer<float> mDetectionBuffer;
    int mDetectionWindowSize = PitchManagerConstants::kDefaultDetectionWindowSize;

    // State
    double mSampleRate = 44100.0;
    float mCurrentPeriod = -1.0f;
};
