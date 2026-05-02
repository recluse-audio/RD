/**
 * test_PitchManager_DataLogger.cpp
 * Verifies PitchManager (DataLogger) writes one CSV row per detect() call.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../TEST_UTILS/TestUtils.h"
#include "../../SOURCE/PITCH/PitchManager.h"
#include "../../SOURCE/CircularBuffer.h"
#include "../../SOURCE/BUFFER_FILLER/BufferFiller.h"
#include <cmath>


namespace
{
    void fillSineBlock (juce::AudioBuffer<float>& buffer, double sampleRate, double freq, juce::int64 startSample)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int s = 0; s < numSamples; ++s)
            {
                const double t = static_cast<double> (startSample + s) / sampleRate;
                d[s] = 0.5f * static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * freq * t));
            }
        }
    }
}


TEST_CASE("PitchManager appends detect_log.csv row per detect() call", "[PitchManager][DataLogger]")
{
    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("PitchManager appends detect_log row")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");

    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    PitchManager manager;
    manager.setDataLogRootDirectory (rootDir);
    manager.setDataLogOutputName    (outputName);

    const double sampleRate  = 48000.0;
    const int    numChannels = 1;
    const int    windowSize  = 2048;
    manager.prepare (sampleRate, numChannels, windowSize);

    // Source buffer: hold several detection windows of audio.
    CircularBuffer source;
    const int circSize = windowSize * 8;
    source.setSize (numChannels, circSize);
    source.clear();

    juce::AudioBuffer<float> block (numChannels, windowSize);
    juce::int64 absSamples = 0;
    for (int i = 0; i < 4; ++i)
    {
        fillSineBlock (block, sampleRate, 220.0, absSamples);
        source.pushBuffer (block);
        absSamples += windowSize;
    }

    auto outputCsv = manager.getDataLogOutputDirectory().getChildFile ("detect_log.csv");

    SECTION ("logging off — no CSV written")
    {
        manager.setIsLogging (false);
        manager.detect (source, 0);
        manager.detect (source, windowSize);
        REQUIRE_FALSE (outputCsv.existsAsFile());
    }

    SECTION ("logging on — one row per detect() call, range + period present")
    {
        manager.setIsLogging (true);

        const int numCalls = 3;
        std::vector<juce::int64> startIndices { 0, windowSize, 2 * windowSize };
        for (int i = 0; i < numCalls; ++i)
            manager.detect (source, startIndices[static_cast<size_t> (i)]);

        REQUIRE (outputCsv.existsAsFile());

        auto lines = juce::StringArray::fromLines (outputCsv.loadFileAsString().trimEnd());
        REQUIRE (lines.size() == numCalls + 1);
        REQUIRE (lines[0] == "detect_call_id,read_start_abs_process_count,read_end_abs_process_count,window_size,period");

        for (int i = 0; i < numCalls; ++i)
        {
            auto cols = juce::StringArray::fromTokens (lines[i + 1], ",", "");
            REQUIRE (cols.size() == 5);
            const auto callId   = cols[0].getLargeIntValue();
            const auto startAbs = cols[1].getLargeIntValue();
            const auto endAbs   = cols[2].getLargeIntValue();
            const auto winSize  = cols[3].getIntValue();
            const auto period   = cols[4].getFloatValue();
            REQUIRE (callId == i + 1);
            REQUIRE (startAbs == startIndices[static_cast<size_t> (i)]);
            REQUIRE (endAbs - startAbs == winSize);
            REQUIRE (winSize == windowSize);
            REQUIRE ((period > 0.0f || period == Catch::Approx (-1.0f)));
        }
    }
}


TEST_CASE("PitchManager logs every detection across Somewhere wav file", "[PitchManager][DataLogger]")
{
    juce::File wavFile = TestUtils::getGoldenDirectory().getChildFile ("GOLDEN_Somewhere_Mono_441k.wav");
    INFO ("Looking for: " << wavFile.getFullPathName());
    REQUIRE (wavFile.existsAsFile());

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (wavFile));
    REQUIRE (reader != nullptr);

    const double sampleRate    = reader->sampleRate;
    const int    numChannels   = static_cast<int> (reader->numChannels);
    const int    totalSamples  = static_cast<int> (reader->lengthInSamples);
    const int    windowSize    = 2048;
    const int    hopSize       = 512;   // overlap: hop < window

    juce::AudioBuffer<float> fullAudio (numChannels, totalSamples);
    REQUIRE (BufferFiller::loadFromWavFile (wavFile, fullAudio));

    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("PitchManager Somewhere full file detection")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");
    const juce::String outputName = "DATA_LOG_OUTPUT_DIR";

    PitchManager manager;
    manager.setDataLogRootDirectory (rootDir);
    manager.setDataLogOutputName    (outputName);
    manager.prepare (sampleRate, numChannels, windowSize);
    manager.setHopSize (hopSize);

    // Circular buffer big enough to hold the entire file so any absolute index reads valid audio.
    CircularBuffer source;
    source.setSize (numChannels, totalSamples + windowSize);
    source.clear();
    source.pushBuffer (fullAudio);

    manager.setIsLogging (true);

    // Iterate detect across the whole file at hop boundaries.
    int expectedCalls = 0;
    for (juce::int64 startAbs = 0; startAbs + windowSize <= totalSamples; startAbs += hopSize)
    {
        manager.detect (source, startAbs);
        ++expectedCalls;
    }
    REQUIRE (expectedCalls > 0);

    auto outputCsv = manager.getDataLogOutputDirectory().getChildFile ("detect_log.csv");
    REQUIRE (outputCsv.existsAsFile());

    auto lines = juce::StringArray::fromLines (outputCsv.loadFileAsString().trimEnd());
    REQUIRE (lines.size() == expectedCalls + 1);
    REQUIRE (lines[0] == "detect_call_id,read_start_abs_process_count,read_end_abs_process_count,window_size,period");

    int rowsWithDetectedPitch = 0;
    for (int i = 0; i < expectedCalls; ++i)
    {
        auto cols = juce::StringArray::fromTokens (lines[i + 1], ",", "");
        REQUIRE (cols.size() == 5);
        const auto startAbs = cols[1].getLargeIntValue();
        const auto endAbs   = cols[2].getLargeIntValue();
        const auto winSize  = cols[3].getIntValue();
        const auto period   = cols[4].getFloatValue();
        REQUIRE (startAbs == static_cast<juce::int64> (i) * hopSize);
        REQUIRE (winSize == windowSize);
        REQUIRE (endAbs - startAbs == winSize);
        if (period > 0.0f)
            ++rowsWithDetectedPitch;
    }

    INFO ("Detected pitch on " << rowsWithDetectedPitch << "/" << expectedCalls << " windows");
    REQUIRE (rowsWithDetectedPitch > 0);

    // Verify the two new per-mark CSVs also produced.
    auto analysisCsv  = manager.getDataLogOutputDirectory().getChildFile ("analysis_marks_log.csv");
    auto synthesisCsv = manager.getDataLogOutputDirectory().getChildFile ("synthesis_marks_log.csv");
    REQUIRE (analysisCsv.existsAsFile());
    REQUIRE (synthesisCsv.existsAsFile());

    auto aLines = juce::StringArray::fromLines (analysisCsv.loadFileAsString().trimEnd());
    auto sLines = juce::StringArray::fromLines (synthesisCsv.loadFileAsString().trimEnd());
    REQUIRE (aLines.size() > 1);
    REQUIRE (sLines.size() > 1);
    REQUIRE (aLines[0] == "detect_call_id,analysis_mark_id,range_start,mark,range_end,period");
    REQUIRE (sLines[0] == "detect_call_id,synthesis_mark_id,synth_range_start,synth_mark,synth_range_end,source_range_start,source_pitch_mark,source_range_end");

    manager.setIsLogging (false);
}
