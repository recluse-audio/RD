#include <catch2/catch_test_macros.hpp>
#include "../SOURCE/PROCESSORS/TDPSOLA/TDPSOLA_Processor.h"
#include "../SOURCE/BufferFiller.h"
#include "TEST_UTILS/TestUtils.h"

TEST_CASE("TDPSOLA_Processor output is not silent after processing sine input", "[TDPSOLA_Processor]")
{
    TestUtils::SetupAndTeardown setup;

    TDPSOLA_Processor processor;

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

        // Compute RMS of this output block
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

TEST_CASE("TDPSOLA_Processor generates correct number of synth marks for given period and shift ratio", "[TDPSOLA_Processor]")
{
    TestUtils::SetupAndTeardown setup;

    TDPSOLA_Processor processor;

    const double sampleRate         = 44100.0;
    const int    blockSize          = 512;
    const int    numChannels        = 2;
    const int    sinePeriod         = 100;  // ~441 Hz
    const int    detectionWindowSize = 2048;  // default detection window size
    const float  shiftRatio         = 1.0f;  // no pitch shift

    processor.prepareToPlay(sampleRate, blockSize);

    // Process enough blocks to trigger at least one detection cycle
    // Detection happens every 2048 samples (detectionWindowSize)
    const int blocksToTriggerDetection = (detectionWindowSize / blockSize) + 1;  // ~5 blocks

    // Process a few more blocks to ensure detection completes and synth marks are generated
    const int numBlocks = blocksToTriggerDetection + 5;

    juce::MidiBuffer midiBuffer;
    double sinePhase = 0.0;  // Track phase continuity across blocks

    for (int block = 0; block < numBlocks; ++block)
    {
        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        // Use phase-continuous version to avoid block boundary artifacts
        sinePhase = BufferFiller::generateSineCycles(buffer, static_cast<double>(sinePeriod), sinePhase);

        processor.processBlock(buffer, midiBuffer);
    }

    // After processing, check that synth marks were generated
    auto& pitchManager = processor.getPitchManager();
    const auto& synthMarks = pitchManager.getSynthMarks();

    // Verify the detected period matches the expected sine wave period
    const float detectedPeriod = pitchManager.getCurrentPeriod();
    const float expectedPeriod = static_cast<float>(sinePeriod);
    const float periodTolerance = expectedPeriod * 0.1f;  // 10% tolerance

    INFO("Expected period: " << expectedPeriod);
    INFO("Detected period: " << detectedPeriod);
    INFO("Period tolerance: " << periodTolerance);

    REQUIRE(detectedPeriod > 0.0f);  // Ensure pitch was detected
    REQUIRE(std::abs(detectedPeriod - expectedPeriod) < periodTolerance);  // Verify correct period

    // Calculate expected number of synth marks
    // New implementation generates marks incrementally across the detection window range
    // Number of marks ≈ windowSize / shiftedPeriod (may vary by ±1 depending on alignment)

    const float shiftedPeriod = detectedPeriod / shiftRatio;
    const int approximateExpectedMarks = static_cast<int>(detectionWindowSize / shiftedPeriod);

    // Verify the actual number of synth marks is close to expected (within ±2 for alignment)
    const int actualSynthMarks = static_cast<int>(synthMarks.size());

    INFO("Shifted period: " << shiftedPeriod);
    INFO("Approximate expected synth marks: " << approximateExpectedMarks);
    INFO("Actual synth marks: " << actualSynthMarks);

    REQUIRE(actualSynthMarks >= approximateExpectedMarks - 2);
    REQUIRE(actualSynthMarks <= approximateExpectedMarks + 2);

    processor.releaseResources();
}

TEST_CASE("TDPSOLA_Processor generates correct number of synth marks with pitch shift", "[TDPSOLA_Processor]")
{
    TestUtils::SetupAndTeardown setup;

    TDPSOLA_Processor processor;

    const double sampleRate         = 44100.0;
    const int    blockSize          = 512;
    const int    numChannels        = 2;
    const int    sinePeriod         = 100;  // ~441 Hz
    const int    detectionWindowSize = 2048;
    const float  shiftRatio         = 0.5f;  // pitch down by octave (half speed)

    processor.prepareToPlay(sampleRate, blockSize);

    // Set the shift ratio parameter (note: ID is "shift_ratio" not "shiftRatio")
    auto& apvts = processor.getAPVTS();
    auto* shiftParam = apvts.getParameter("shift_ratio");
    REQUIRE(shiftParam != nullptr);  // Ensure parameter exists
    shiftParam->setValueNotifyingHost(shiftParam->convertTo0to1(shiftRatio));

    const int blocksToTriggerDetection = (detectionWindowSize / blockSize) + 1;
    const int numBlocks = blocksToTriggerDetection + 5;

    juce::MidiBuffer midiBuffer;
    double sinePhase = 0.0;  // Track phase continuity across blocks

    for (int block = 0; block < numBlocks; ++block)
    {
        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        // Use phase-continuous version to avoid block boundary artifacts
        sinePhase = BufferFiller::generateSineCycles(buffer, static_cast<double>(sinePeriod), sinePhase);

        processor.processBlock(buffer, midiBuffer);
    }

    auto& pitchManager = processor.getPitchManager();
    const auto& synthMarks = pitchManager.getSynthMarks();

    // Verify the detected period matches the expected sine wave period
    const float detectedPeriod = pitchManager.getCurrentPeriod();
    const float expectedPeriod = static_cast<float>(sinePeriod);
    const float periodTolerance = expectedPeriod * 0.1f;  // 10% tolerance

    INFO("Expected period: " << expectedPeriod);
    INFO("Detected period: " << detectedPeriod);
    INFO("Period tolerance: " << periodTolerance);

    REQUIRE(detectedPeriod > 0.0f);  // Ensure pitch was detected
    REQUIRE(std::abs(detectedPeriod - expectedPeriod) < periodTolerance);  // Verify correct period

    // With shiftRatio = 0.5, shiftedPeriod = period / 0.5 = period * 2
    // This means fewer synth marks (larger spacing)
    const float shiftedPeriod = detectedPeriod / shiftRatio;
    const int approximateExpectedMarks = static_cast<int>(detectionWindowSize / shiftedPeriod);

    const int actualSynthMarks = static_cast<int>(synthMarks.size());

    INFO("Shift ratio: " << shiftRatio);
    INFO("Shifted period: " << shiftedPeriod);
    INFO("Approximate expected synth marks: " << approximateExpectedMarks);
    INFO("Actual synth marks: " << actualSynthMarks);

    REQUIRE(actualSynthMarks >= approximateExpectedMarks - 2);
    REQUIRE(actualSynthMarks <= approximateExpectedMarks + 2);

    processor.releaseResources();
}
