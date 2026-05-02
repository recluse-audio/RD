#include <catch2/catch_test_macros.hpp>
#include "../../../SOURCE/PROCESSORS/GRAIN/GrainShifterProcessor.h"
#include "../../../SOURCE/BUFFER_FILLER/BufferFiller.h"
#include "../../TEST_UTILS/TestUtils.h"

TEST_CASE("GrainShifterProcessor output is not silent after processing sine input", "[GrainShifterProcessor]")
{
    TestUtils::SetupAndTeardown setup;

    GrainShifterProcessor processor;

    const double sampleRate    = 44100.0;
    const int    blockSize     = 512;
    const int    numChannels   = 2;
    const int    sinePeriod    = 100; // ~441 Hz

    processor.prepareToPlay(sampleRate, blockSize);

    // Push enough blocks to exceed the detection window (2048 samples) plus
    // the lookahead (~2205 samples at 44100 Hz) so that grains are generated
    // and have their write positions fall within the queried output range.
    const int numBlocks = 30; // 30 * 512 = 15360 samples — well past both thresholds

    float peakRMS = 0.0f;

    juce::MidiBuffer midiBuffer;

    for (int block = 0; block < numBlocks; ++block)
    {
        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        BufferFiller::generateSineCycles(buffer, sinePeriod);

        processor.processBlock(buffer, midiBuffer);

        float sumSq = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < blockSize; ++i)
                sumSq += data[i] * data[i];
        }
        float rms = std::sqrt(sumSq / static_cast<float>(numChannels * blockSize));
        peakRMS = std::max(peakRMS, rms);
    }

    processor.releaseResources();

    REQUIRE(peakRMS > 0.0f);
}

TEST_CASE("GrainShifterProcessor defers window/hop changes to audio thread", "[GrainShifterProcessor]")
{
    TestUtils::SetupAndTeardown setup;

    GrainShifterProcessor processor;

    const double sampleRate  = 48000.0;
    const int    blockSize   = 512;
    const int    numChannels = 2;

    processor.prepareToPlay(sampleRate, blockSize);

    auto& pm = processor.getPitchManager();

    const int initialWindow = pm.getDetectionWindowSize();
    const int initialHop    = pm.getHopSize();

    // Defaults from createParameterLayout: window idx 1 = 1024, hop idx 1 = 512.
    CHECK(initialWindow == 1024);
    CHECK(initialHop    == 512);

    // Choice indices: window {512,1024,2048,4096,8192}, hop {256,512,1024,2048,4096}.
    SECTION("window size change is staged on message thread, applied on audio thread")
    {
        // idx 2 -> 2048
        processor.parameterChanged("pitch_window_size", 2.0f);

        // Must not have touched the PitchManager from the listener call.
        CHECK(pm.getDetectionWindowSize() == initialWindow);

        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);

        CHECK(pm.getDetectionWindowSize() == 2048);
    }

    SECTION("hop size change is staged on message thread, applied on audio thread")
    {
        // idx 0 -> 256
        processor.parameterChanged("pitch_hop_size", 0.0f);

        CHECK(pm.getHopSize() == initialHop);

        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);

        CHECK(pm.getHopSize() == 256);
    }

    SECTION("repeated changes coalesce — only final value is applied")
    {
        processor.parameterChanged("pitch_window_size", 0.0f); // 512
        processor.parameterChanged("pitch_window_size", 3.0f); // 4096
        processor.parameterChanged("pitch_window_size", 2.0f); // 2048

        CHECK(pm.getDetectionWindowSize() == initialWindow);

        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);

        CHECK(pm.getDetectionWindowSize() == 2048);
    }

    processor.releaseResources();
}

TEST_CASE("GrainShifterProcessor generates correct number of synth marks for given period and shift ratio", "[GrainShifterProcessor]")
{
    TestUtils::SetupAndTeardown setup;

    GrainShifterProcessor processor;

    const double sampleRate          = 44100.0;
    const int    blockSize           = 512;
    const int    numChannels         = 2;
    const int    sinePeriod          = 50;  //
    const float  shiftRatio          = 1.0f;

    processor.prepareToPlay(sampleRate, blockSize);

    const int detectionWindowSize = processor.getPitchManager().getDetectionWindowSize();
    const int hopSize             = processor.getPitchManager().getHopSize();
    const int blocksToTriggerDetection = (detectionWindowSize / blockSize) + 1;
    const int numBlocks = blocksToTriggerDetection + 5;

    juce::MidiBuffer midiBuffer;
    double sinePhase = 0.0;

    for (int block = 0; block < numBlocks; ++block)
    {
        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        sinePhase = BufferFiller::generateSineCycles(buffer, static_cast<double>(sinePeriod), sinePhase);
        processor.processBlock(buffer, midiBuffer);
    }

    auto& pitchManager = processor.getPitchManager();
    const auto& synthMarks = pitchManager.getSynthMarks();

    const float detectedPeriod  = pitchManager.getCurrentPeriod();
    const float expectedPeriod  = static_cast<float>(sinePeriod);
    const float periodTolerance = expectedPeriod * 0.1f;

    INFO("Expected period: " << expectedPeriod);
    INFO("Detected period: " << detectedPeriod);

    REQUIRE(detectedPeriod > 0.0f);
    REQUIRE(std::abs(detectedPeriod - expectedPeriod) < periodTolerance);

    // SynthMarker clears its mark list at the start of each generateSynthMarks() call,
    // so only marks emitted during the last detect (covering hopSize samples) remain.
    const float shiftedPeriod           = detectedPeriod / shiftRatio;
    const int   approximateExpectedMarks = static_cast<int>(hopSize / shiftedPeriod);
    const int   actualSynthMarks         = static_cast<int>(synthMarks.size());

    INFO("Approximate expected synth marks: " << approximateExpectedMarks);
    INFO("Actual synth marks: " << actualSynthMarks);

    REQUIRE(actualSynthMarks >= approximateExpectedMarks - 2);
    REQUIRE(actualSynthMarks <= approximateExpectedMarks + 2);

    processor.releaseResources();
}

TEST_CASE("GrainShifterProcessor generates correct number of synth marks with pitch shift", "[GrainShifterProcessor]")
{
    TestUtils::SetupAndTeardown setup;

    GrainShifterProcessor processor;

    const double sampleRate          = 44100.0;
    const int    blockSize           = 512;
    const int    numChannels         = 2;
    const int    sinePeriod          = 100;
    const float  shiftRatio          = 0.5f;  // pitch down by octave

    processor.prepareToPlay(sampleRate, blockSize);

    auto& apvts = processor.getAPVTS();
    auto* shiftParam = apvts.getParameter("shift_ratio");
    REQUIRE(shiftParam != nullptr);
    shiftParam->setValueNotifyingHost(shiftParam->convertTo0to1(shiftRatio));

    const int detectionWindowSize = processor.getPitchManager().getDetectionWindowSize();
    const int hopSize             = processor.getPitchManager().getHopSize();
    const int blocksToTriggerDetection = (detectionWindowSize / blockSize) + 1;
    const int numBlocks = blocksToTriggerDetection + 5;

    juce::MidiBuffer midiBuffer;
    double sinePhase = 0.0;

    for (int block = 0; block < numBlocks; ++block)
    {
        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        sinePhase = BufferFiller::generateSineCycles(buffer, static_cast<double>(sinePeriod), sinePhase);
        processor.processBlock(buffer, midiBuffer);
    }

    auto& pitchManager = processor.getPitchManager();
    const auto& synthMarks = pitchManager.getSynthMarks();

    const float detectedPeriod  = pitchManager.getCurrentPeriod();
    const float expectedPeriod  = static_cast<float>(sinePeriod);
    const float periodTolerance = expectedPeriod * 0.1f;

    REQUIRE(detectedPeriod > 0.0f);
    REQUIRE(std::abs(detectedPeriod - expectedPeriod) < periodTolerance);

    // With shiftRatio = 0.5, shiftedPeriod = period / 0.5 = period * 2 → fewer marks.
    // SynthMarker clears on each call, so only the last hopSize-worth of marks remain.
    const float shiftedPeriod           = detectedPeriod / shiftRatio;
    const int   approximateExpectedMarks = static_cast<int>(hopSize / shiftedPeriod);
    const int   actualSynthMarks         = static_cast<int>(synthMarks.size());

    REQUIRE(actualSynthMarks >= approximateExpectedMarks - 2);
    REQUIRE(actualSynthMarks <= approximateExpectedMarks + 2);

    processor.releaseResources();
}
