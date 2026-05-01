/**
 * Granulator.cpp
 * Created by Ryan Devens
 *
 * Implementation of Granulator.
 */

#include "Granulator.h"

Granulator::Granulator(CircularBuffer& sourceBuffer)
    : mSourceBuffer(sourceBuffer)
{
}

Granulator::~Granulator()
{
}

void Granulator::prepare(double sampleRate, int numChannels, juce::int64 lookaheadSamples, int maxGrains)
{
    mSampleRate       = sampleRate;
    mNumChannels      = numChannels;
    mLookaheadSamples = lookaheadSamples;
    mMaxGrains        = maxGrains;

    // Configure the shared window with Tukey shape for smooth grain edges
    // Window size set to accommodate maximum expected grain size (2048 samples covers pitch down to ~22 Hz at 44.1kHz)
    // Period will be set per-grain based on actual grain size in Grain::process()
    const int maxWindowSize = 2048;
    mWindow.setSizeShapePeriod(maxWindowSize, Window::Shape::kTukey, maxWindowSize);

    // Pre-allocate grain pool — all grains share the same window reference
    mGrains.clear();
    mGrains.reserve(maxGrains);

    for (int i = 0; i < maxGrains; ++i)
    {
        mGrains.emplace_back(SynthMark(), mWindow, mSourceBuffer, mLookaheadSamples);
    }
}

void Granulator::generateGrains(const std::vector<SynthMark>& synthMarks)
{
    int synthMarkIndex = 0;

    for (auto& grain : mGrains)
    {
        if (synthMarkIndex >= static_cast<int>(synthMarks.size()))
            break;

        // Only reuse grains that are finished or invalid
        if (!grain.isValid() || grain.isFinished())
        {
            grain.setGrain(synthMarks[synthMarkIndex], mLookaheadSamples);
            ++synthMarkIndex;
        }
    }

    jassert(synthMarkIndex == static_cast<int>(synthMarks.size()) &&
            "Not enough available grains - increase maxGrains or implement voice stealing");

    // Snapshot synth marks for per-grain logging (only the ones actually assigned).
    if (getIsLogging())
    {
        mLastSynthMarks.assign (synthMarks.begin(),
                                synthMarks.begin() + synthMarkIndex);
        mGenerateLogPending = true;
        logData();
    }
}

bool Granulator::doLogData()
{
    if (! mGenerateLogPending)
        return true;
    mGenerateLogPending = false;

    auto file = getDataLogOutputDirectory().getChildFile ("synthesis_grains.csv");
    const bool needsHeader = ! file.existsAsFile();

    juce::String contents;
    contents.preallocateBytes (static_cast<size_t> (128 + 96 * mLastSynthMarks.size()));

    if (needsHeader)
        contents << "source_analysis_id,source_start,source_center,source_end,"
                    "grain_id,start_sample,center_sample,end_sample,"
                    "source_period,synthesis_period,duration_samples,window_alpha\n";

    const int N = static_cast<int> (mLastSynthMarks.size());
    for (int i = 0; i < N; ++i)
    {
        const auto& sm = mLastSynthMarks[static_cast<size_t> (i)];
        if (! sm.isValid())
            continue;

        // Each unique source pitch-mark center gets a stable analysis id.
        int analysisId;
        auto it = mAnalysisIdByPitchCenter.find (sm.pitchMark);
        if (it != mAnalysisIdByPitchCenter.end())
        {
            analysisId = it->second;
        }
        else
        {
            analysisId = mNextAnalysisId++;
            mAnalysisIdByPitchCenter.emplace (sm.pitchMark, analysisId);
        }

        const juce::int64 sourcePeriod = sm.pitchRangeEnd - sm.pitchMark;

        // synthesis_period = inter-mark hop in synth-time. Use forward delta when
        // next mark exists in this batch; otherwise fall back to backward delta.
        juce::int64 synthesisPeriod;
        if (i + 1 < N)
            synthesisPeriod = mLastSynthMarks[static_cast<size_t> (i + 1)].synthMark - sm.synthMark;
        else if (i > 0)
            synthesisPeriod = sm.synthMark - mLastSynthMarks[static_cast<size_t> (i - 1)].synthMark;
        else
            synthesisPeriod = sourcePeriod; // single-mark batch fallback

        const juce::int64 duration = sm.synthRangeEnd - sm.synthRangeStart;

        contents << analysisId           << ","
                 << sm.pitchRangeStart   << ","
                 << sm.pitchMark         << ","
                 << sm.pitchRangeEnd     << ","
                 << mNextGrainId++       << ","
                 << sm.synthRangeStart   << ","
                 << sm.synthMark         << ","
                 << sm.synthRangeEnd     << ","
                 << sourcePeriod         << ","
                 << synthesisPeriod      << ","
                 << duration             << ","
                 << juce::String (mWindowAlpha, 2) << "\n";
    }

    juce::FileOutputStream stream (file);
    if (! stream.openedOk())
        return false;

    stream.setPosition (stream.getFile().getSize());
    stream.writeText (contents, false, false, nullptr);
    return true;
}

void Granulator::process(juce::AudioBuffer<float>& outputBuffer, juce::int64 blockStartSample, juce::int64 blockEndSample)
{
    for (auto& grain : mGrains)
    {
        grain.process(outputBuffer, blockStartSample, blockEndSample);
    }
}

void Granulator::reset()
{
    for (auto& grain : mGrains)
    {
        grain.setGrain(SynthMark(), mLookaheadSamples);
    }
}
