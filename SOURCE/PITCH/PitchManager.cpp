/**
 * PitchManager.cpp
 * Created by Ryan Devens
 */

#include "PitchManager.h"
#include <algorithm>

//=======================================
PitchManager::PitchManager()
{
    mPitchMarker = std::make_unique<PitchMarker>();
    mSynthMarker = std::make_unique<SynthMarker>();
}

//=======================================
PitchManager::~PitchManager()
{

}

//=======================================
void PitchManager::prepare(double sampleRate, int numChannels, int detectionWindowSize)
{
    mSampleRate = sampleRate;
    mDetectionWindowSize.set(detectionWindowSize);

    mPitchDetector.prepare(sampleRate);

    // Allocate to max so subsequent setDetectionWindowSize calls never reallocate.
    mDetectionBuffer.setSize(numChannels, PitchManagerConstants::kMaxDetectionWindowSize);
    mDetectionBuffer.clear();
    mDetectionBuffer.setSize(numChannels, detectionWindowSize, false, false, true);

    reset();
}

void PitchManager::setDetectionWindowSize(int newSize)
{
    newSize = juce::jlimit(64, PitchManagerConstants::kMaxDetectionWindowSize, newSize);
    mDetectionWindowSize.set(newSize);

    const int numChannels = mDetectionBuffer.getNumChannels();
    if (numChannels > 0)
        mDetectionBuffer.setSize(numChannels, newSize, false, false, true);
}

void PitchManager::setHopSize(int newSize)
{
    newSize = juce::jmax(1, newSize);
    mHopSize.set(newSize);
}

//=======================================
void PitchManager::reset()
{
    mCurrentPeriod = -1.0f;
    mDetectionBuffer.clear();
    mPitchMarker->reset();
    mSynthMarker->reset();
}

//=======================================
float PitchManager::detect(CircularBuffer& circularBuffer, juce::int64 startAbsIndex, float shiftRatio)
{
    const int wrappedStart = circularBuffer.getWrappedIndex(startAbsIndex);
    circularBuffer.readRange(mDetectionBuffer, wrappedStart);

    mCurrentPeriod = mPitchDetector.process(mDetectionBuffer);

    // if (mCurrentPeriod <= 0.0f)
    //     return mCurrentPeriod;

    // Generate pitch marks across this detection window
    const int windowSize = mDetectionWindowSize.get();
    const juce::int64 windowEnd = startAbsIndex + windowSize;
    const juce::Range<juce::int64> windowRange(startAbsIndex, windowEnd);
    const int maxIterations = static_cast<int>(windowSize / mCurrentPeriod) + 4;

    for (int i = 0; i < maxIterations; ++i)
    {
        const juce::int64 predicted = mPitchMarker->getPredictedNextMark();
        if (predicted > 0 && predicted >= windowEnd)
            break;

        const juce::int64 mark = mPitchMarker->doPitchMarking(circularBuffer, windowRange, mCurrentPeriod, windowEnd, true);

        if (mark < 0 || mark >= windowEnd)
            break;
    }

    // Generate synth marks using the shifted output period.
    // Only feed pitch marks that fall within this detection window — the FIFO
    // holds up to 32 historical marks, but synth-mark generation must be scoped
    // to current window content.
    const float safeShift     = std::max(shiftRatio, 0.01f);
    const float shiftedPeriod = mCurrentPeriod / safeShift;
    const auto windowPitchMarks = mPitchMarker->getPitchMarksInRange(windowRange);
    mSynthMarker->generateSynthMarks(windowPitchMarks, shiftedPeriod, windowRange);

    // Snapshot range info for data logging.
    mLastDetectStartAbs   = startAbsIndex;
    mLastDetectEndAbs     = windowEnd;
    mLastDetectWindowSize = windowSize;
    mLastDetectedPeriod   = mCurrentPeriod;
    // FIFO-backed vectors have fixed .size() (capped at FIFO max), so report
    // only the marks that fall within this detect() window.
    mLastPitchMarkCount   = mPitchMarker->getPitchMarksInRange (windowRange).size();
    mLastSynthMarkCount   = mSynthMarker->getSynthMarksInRange (windowRange).size();

    if (getIsLogging())
    {
        mDetectLogPending = true;
        logData();
    }

    return mCurrentPeriod;
}

//=======================================
bool PitchManager::doLogData()
{
    if (! mDetectLogPending)
        return true;
    mDetectLogPending = false;

    auto file = getDataLogOutputDirectory().getChildFile ("detect_log.csv");
    const bool needsHeader = ! file.existsAsFile();

    juce::String row;
    if (needsHeader)
        row << "start_abs,end_abs,window_size,period,num_pitch_marks,num_synth_marks\n";

    row << juce::String (mLastDetectStartAbs)   << ","
        << juce::String (mLastDetectEndAbs)     << ","
        << juce::String (mLastDetectWindowSize) << ","
        << juce::String (mLastDetectedPeriod, 6) << ","
        << juce::String (static_cast<juce::int64> (mLastPitchMarkCount)) << ","
        << juce::String (static_cast<juce::int64> (mLastSynthMarkCount)) << "\n";

    juce::FileOutputStream stream (file);
    if (! stream.openedOk())
        return false;

    stream.setPosition (stream.getFile().getSize());
    stream.writeText (row, false, false, nullptr);
    return true;
}
