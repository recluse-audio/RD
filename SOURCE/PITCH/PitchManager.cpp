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
namespace
{
    void appendCsv (const juce::File& file, const juce::String& header, const juce::String& rows)
    {
        const bool needsHeader = ! file.existsAsFile();
        juce::FileOutputStream stream (file);
        if (! stream.openedOk())
            return;
        stream.setPosition (stream.getFile().getSize());
        if (needsHeader)
            stream.writeText (header, false, false, nullptr);
        stream.writeText (rows, false, false, nullptr);
    }
}

//=======================================
float PitchManager::detect(CircularBuffer& circularBuffer, juce::int64 startAbsIndex, float shiftRatio)
{
    const int wrappedStart = circularBuffer.getWrappedIndex(startAbsIndex);
    circularBuffer.readRange(mDetectionBuffer, wrappedStart);

    mCurrentPeriod = mPitchDetector.process(mDetectionBuffer);

    const int windowSize = mDetectionWindowSize.get();
    const juce::int64 windowEnd = startAbsIndex + windowSize;
    const juce::Range<juce::int64> windowRange(startAbsIndex, windowEnd);

    // ---- 1. Detect event ----
    mLastDetectStartAbs   = startAbsIndex;
    mLastDetectEndAbs     = windowEnd;
    mLastDetectWindowSize = windowSize;
    mLastDetectedPeriod   = mCurrentPeriod;
    ++mLastDetectCallId;
    if (getIsLogging())
    {
        mPendingLogEvent = LogEvent::kDetect;
        logData();
    }

    // Generate pitch marks across this detection window.
    const int maxIterations = static_cast<int>(windowSize / mCurrentPeriod) + 4;
    mLastAnalysisMarks.clear();
    for (int i = 0; i < maxIterations; ++i)
    {
        const juce::int64 mark = mPitchMarker->doPitchMarking(circularBuffer, windowRange, mCurrentPeriod, windowEnd, true);
        if (mark < 0)
            break;
        mLastAnalysisMarks.emplace_back (mark, mCurrentPeriod);
    }

    // ---- 2. AnalysisMarks event ----
    if (getIsLogging())
    {
        mPendingLogEvent = LogEvent::kAnalysisMarks;
        logData();
    }

    // Generate synth marks using the shifted output period.
    const float safeShift     = std::max(shiftRatio, 0.01f);
    const float shiftedPeriod = mCurrentPeriod / safeShift;
    mSynthMarker->generateSynthMarks(mPitchMarker->getPitchMarks(), shiftedPeriod, windowRange);

    // Snapshot synth marks emitted this call (SynthMarker clears on each call).
    mLastSynthesisMarks = mSynthMarker->getSynthMarks();

    // ---- 3. SynthesisMarks event ----
    if (getIsLogging())
    {
        mPendingLogEvent = LogEvent::kSynthesisMarks;
        logData();
    }

    return mCurrentPeriod;
}

//=======================================
bool PitchManager::doLogData()
{
    const LogEvent ev = mPendingLogEvent;
    mPendingLogEvent = LogEvent::kIdle;

    switch (ev)
    {
        case LogEvent::kDetect:
        {
            auto file = getDataLogOutputDirectory().getChildFile ("detect_log.csv");
            const juce::String header = "detect_call_id,start_abs,end_abs,window_size,period\n";
            juce::String row;
            row << juce::String (mLastDetectCallId)     << ","
                << juce::String (mLastDetectStartAbs)   << ","
                << juce::String (mLastDetectEndAbs)     << ","
                << juce::String (mLastDetectWindowSize) << ","
                << juce::String (mLastDetectedPeriod, 6) << "\n";
            appendCsv (file, header, row);
            return true;
        }

        case LogEvent::kAnalysisMarks:
        {
            auto file = getDataLogOutputDirectory().getChildFile ("analysis_marks_log.csv");
            const juce::String header = "detect_call_id,analysis_mark_id,range_start,mark,range_end,period\n";
            juce::String rows;
            rows.preallocateBytes (64 * mLastAnalysisMarks.size() + 64);
            for (const auto& pm : mLastAnalysisMarks)
            {
                rows << juce::String (mLastDetectCallId)        << ","
                     << juce::String (mNextAnalysisMarkId++)    << ","
                     << juce::String (pm.rangeStart)            << ","
                     << juce::String (pm.mark)                  << ","
                     << juce::String (pm.rangeEnd)              << ","
                     << juce::String (mLastDetectedPeriod, 6)   << "\n";
            }
            appendCsv (file, header, rows);
            return true;
        }

        case LogEvent::kSynthesisMarks:
        {
            auto file = getDataLogOutputDirectory().getChildFile ("synthesis_marks_log.csv");
            const juce::String header = "detect_call_id,synthesis_mark_id,synth_range_start,synth_mark,synth_range_end,source_range_start,source_pitch_mark,source_range_end\n";
            juce::String rows;
            rows.preallocateBytes (96 * mLastSynthesisMarks.size() + 96);
            for (const auto& sm : mLastSynthesisMarks)
            {
                if (! sm.isValid())
                    continue;
                rows << juce::String (mLastDetectCallId)         << ","
                     << juce::String (mNextSynthesisMarkId++)    << ","
                     << juce::String (sm.synthRangeStart)        << ","
                     << juce::String (sm.synthMark)              << ","
                     << juce::String (sm.synthRangeEnd)          << ","
                     << juce::String (sm.pitchRangeStart)        << ","
                     << juce::String (sm.pitchMark)              << ","
                     << juce::String (sm.pitchRangeEnd)          << "\n";
            }
            appendCsv (file, header, rows);
            return true;
        }

        case LogEvent::kIdle:
        default:
            return true; // parent cascade with no pending event — no-op
    }
}
