/**
 * test_Granulator_DataLogger.cpp
 * End-to-end: run PitchManager.detect + Granulator.generateGrains across
 * the Somewhere wav file and verify synthesis_grains.csv accumulates per-grain
 * rows in the TD-PSOLA reference format.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../TEST_UTILS/TestUtils.h"
#include "../../SOURCE/PROCESSORS/GRAIN/Granulator.h"
#include "../../SOURCE/PITCH/PitchManager.h"
#include "../../SOURCE/CircularBuffer.h"
#include "../../SOURCE/BUFFER_FILLER/BufferFiller.h"


TEST_CASE("Granulator end-to-end: Somewhere wav writes synthesis_grains.csv", "[Granulator][DataLogger]")
{
    juce::File wavFile = TestUtils::getGoldenDirectory().getChildFile ("GOLDEN_Somewhere_Mono_441k.wav");
    INFO ("Looking for: " << wavFile.getFullPathName());
    REQUIRE (wavFile.existsAsFile());

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (wavFile));
    REQUIRE (reader != nullptr);

    const double sampleRate   = reader->sampleRate;
    const int    numChannels  = static_cast<int> (reader->numChannels);
    const int    totalSamples = static_cast<int> (reader->lengthInSamples);
    const int    windowSize   = 2048;
    const int    hopSize      = 512;
    const float  shiftRatio   = 1.5f;

    juce::AudioBuffer<float> fullAudio (numChannels, totalSamples);
    REQUIRE (BufferFiller::loadFromWavFile (wavFile, fullAudio));

    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("Granulator Somewhere end-to-end")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");
    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    // Source buffer holds entire file so any absolute index reads valid audio.
    CircularBuffer source;
    source.setSize (numChannels, totalSamples + windowSize);
    source.clear();
    source.pushBuffer (fullAudio);

    PitchManager pitchManager;
    pitchManager.prepare (sampleRate, numChannels, windowSize);
    pitchManager.setHopSize (hopSize);

    Granulator granulator (source);
    granulator.setDataLogRootDirectory (rootDir);
    granulator.setDataLogOutputName    (outputName);
    granulator.prepare (sampleRate, numChannels, /*lookahead*/ 0, /*maxGrains*/ 256);

    granulator.setIsLogging (true);

    int detectCalls          = 0;
    int generateGrainsCalls  = 0;
    int totalSynthMarksFed   = 0;

    constexpr int kMaxGrainsToProcess = 128;

    for (juce::int64 startAbs = 0; startAbs + windowSize <= totalSamples; startAbs += hopSize)
    {
        pitchManager.detect (source, startAbs, shiftRatio);
        ++detectCalls;

        const juce::Range<juce::int64> windowRange (startAbs, startAbs + windowSize);
        auto synthMarks = pitchManager.getSynthMarksInRange (windowRange);

        if (! synthMarks.empty())
        {
            const int remaining = kMaxGrainsToProcess - totalSynthMarksFed;
            if (remaining <= 0)
                break;

            if (static_cast<int> (synthMarks.size()) > remaining)
                synthMarks.resize (static_cast<size_t> (remaining));

            granulator.generateGrains (synthMarks);
            ++generateGrainsCalls;
            totalSynthMarksFed += static_cast<int> (synthMarks.size());

            if (totalSynthMarksFed >= kMaxGrainsToProcess)
                break;
        }
    }

    INFO ("detect calls: " << detectCalls
          << ", generateGrains calls: " << generateGrainsCalls
          << ", total synth marks fed: " << totalSynthMarksFed);

    REQUIRE (detectCalls > 0);
    REQUIRE (generateGrainsCalls > 0);
    REQUIRE (totalSynthMarksFed > 0);

    auto outputCsv = granulator.getDataLogOutputDirectory().getChildFile ("synthesis_grains.csv");
    REQUIRE (outputCsv.existsAsFile());

    auto lines = juce::StringArray::fromLines (outputCsv.loadFileAsString().trimEnd());
    REQUIRE (lines.size() >= 2); // header + at least one grain
    REQUIRE (lines[0] == "source_analysis_id,source_start,source_center,source_end,"
                         "grain_id,start_sample,center_sample,end_sample,"
                         "source_period,synthesis_period,duration_samples,window_alpha");

    // Each row past the header is one grain; sanity-check shape and monotonic grain_id.
    int prevGrainId = -1;
    for (int i = 1; i < lines.size(); ++i)
    {
        auto cols = juce::StringArray::fromTokens (lines[i], ",", "");
        REQUIRE (cols.size() == 12);

        const int analysisId   = cols[0].getIntValue();
        const auto srcStart    = cols[1].getLargeIntValue();
        const auto srcCenter   = cols[2].getLargeIntValue();
        const auto srcEnd      = cols[3].getLargeIntValue();
        const int grainId      = cols[4].getIntValue();
        const auto synStart    = cols[5].getLargeIntValue();
        const auto synCenter   = cols[6].getLargeIntValue();
        const auto synEnd      = cols[7].getLargeIntValue();
        const auto srcPeriod   = cols[8].getLargeIntValue();
        const auto synPeriod   = cols[9].getLargeIntValue();
        const auto duration    = cols[10].getLargeIntValue();
        const float winAlpha   = cols[11].getFloatValue();

        INFO ("row " << i << ": " << lines[i]);
        REQUIRE (analysisId >= 0);
        REQUIRE (grainId == prevGrainId + 1); // strictly increasing
        prevGrainId = grainId;

        REQUIRE (srcStart  <= srcCenter);
        REQUIRE (srcCenter <= srcEnd);
        REQUIRE (synStart  <= synCenter);
        REQUIRE (synCenter <= synEnd);
        REQUIRE (srcPeriod > 0);
        REQUIRE (synPeriod > 0);
        REQUIRE (duration  == synEnd - synStart);
        REQUIRE (winAlpha == Catch::Approx (0.5f).margin (0.001f));
    }

    // Reference TD-PSOLA at shift_ratio=1.5 produces ~2/3 the source period in synth.
    // Verify aggregate synthesis_period < source_period across all rows.
    juce::int64 sumSrc = 0, sumSyn = 0;
    int counted = 0;
    for (int i = 1; i < lines.size(); ++i)
    {
        auto cols = juce::StringArray::fromTokens (lines[i], ",", "");
        sumSrc += cols[8].getLargeIntValue();
        sumSyn += cols[9].getLargeIntValue();
        ++counted;
    }
    INFO ("avg src period: " << (counted ? sumSrc / counted : 0)
          << "  avg syn period: " << (counted ? sumSyn / counted : 0));
    REQUIRE (sumSyn < sumSrc); // pitched up → smaller synth period

    granulator.setIsLogging (false);
}


TEST_CASE("Granulator does not write CSV when logging off", "[Granulator][DataLogger]")
{
    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("Granulator no log when off")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");

    CircularBuffer source;
    source.setSize (1, 4096);
    source.clear();

    Granulator granulator (source);
    granulator.setDataLogRootDirectory (rootDir);
    granulator.setDataLogOutputName    ("DATA_LOG_OUTPUT_DIR");
    granulator.prepare (44100.0, 1, 0, 64);

    granulator.setIsLogging (false);

    SynthMark sm;
    sm.pitchMark = 12; sm.pitchRangeStart = 0; sm.pitchRangeEnd = 37;
    sm.synthMark = 12; sm.synthRangeStart = 0; sm.synthRangeEnd = 37;
    sm.isVoiced = true;
    std::vector<SynthMark> marks { sm };
    granulator.generateGrains (marks);

    auto outputCsv = granulator.getDataLogOutputDirectory().getChildFile ("synthesis_grains.csv");
    REQUIRE_FALSE (outputCsv.existsAsFile());
}
