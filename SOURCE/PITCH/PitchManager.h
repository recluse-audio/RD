/**
 * PitchManager.h
 * Created by Ryan Devens
 *
 * Manages pitch detection and pitch mark tracking.
 * Coordinates TD_PitchDetector and PitchMarker, tracks absolute sample position,
 * and stores pitch marks in a FIFO buffer.
 */

#pragma once
#include <vector>

#include "Util/Juce_Header.h"
#include "TD_PitchDetector.h"
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
 * - Accumulates incoming audio until detection window is filled
 * - Runs pitch detection when enough samples are available
 * - Tracks pitch marks using PitchMarker
 * - Stores pitch marks in a circular FIFO buffer
 * - Tracks absolute sample position across all processed audio
 *
 * Usage:
 * 1. Call prepare() with sample rate
 * 2. Call process() with each audio buffer
 * 3. Query getCurrentPeriod() to get detected period
 * 4. Use findPitchMark() to locate pitch marks in the circular buffer
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
    void prepare(double sampleRate, int detectionWindowSize = PitchManagerConstants::kDefaultDetectionWindowSize);

    /**
     * Process an audio buffer.
     * Accumulates audio and runs pitch detection when enough samples are available.
     *
     * @param buffer Input audio buffer (any size)
     * @param circularBuffer Circular buffer to write audio to (for pitch mark detection)
     * @return True if pitch was detected this call, false otherwise
     */
    bool process(const juce::AudioBuffer<float>& buffer, CircularBuffer& circularBuffer);

    /**
     * Perform pitch marking on the circular buffer.
     * Uses PitchMarker with the most recently detected period.
     * The pitch mark is stored in the PitchMarker's internal FIFO.
     *
     * @param circularBuffer Circular buffer to search
     * @param searchRange Range to search for pitch mark (absolute sample count)
     * @param usePrediction Whether to use prediction from previous marks
     * @return Pitch mark position in absolute sample count, or -1 if no valid period
     */
    juce::int64 findPitchMark(const CircularBuffer& circularBuffer, juce::Range<juce::int64> searchRange, bool usePrediction = true);

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
     * Get the absolute sample counter.
     * @return Total samples processed since prepare() or reset()
     */
    juce::int64 getAbsoluteSampleCount() const { return mAbsoluteSampleCounter; }

    /**
     * Check if we have enough samples accumulated for detection.
     * @return True if detection will run on next process() call
     */
    bool isReadyForDetection() const { return mDetectionBufferFillPos >= mDetectionWindowSize; }

    /**
     * Reset all state.
     */
    void reset();

    /**
     * Get access to the pitch detector for configuration.
     */
    TD_PitchDetector& getPitchDetector() { return mPitchDetector; }

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
    TD_PitchDetector mPitchDetector;
    std::unique_ptr<PitchMarker> mPitchMarker;
    std::unique_ptr<SynthMarker> mSynthMarker;

    // Detection buffer (accumulates samples until full)
    juce::AudioBuffer<float> mDetectionBuffer;
    int mDetectionBufferFillPos = 0;
    int mDetectionWindowSize = PitchManagerConstants::kDefaultDetectionWindowSize;

    // State
    double mSampleRate = 44100.0;
    juce::int64 mAbsoluteSampleCounter = 0;
    float mCurrentPeriod = -1.0f;

    /**
     * Run pitch detection on the accumulated detection buffer.
     * @return Detected period, or -1 if no pitch detected
     */
    float _runDetection();
};
