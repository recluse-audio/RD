#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/GAIN/GainProcessor.h"
#include "../../../SOURCE/BUFFER_FILLER/BufferFiller.h"

//========================================================
//===================== DATA LOGGING =====================
//========================================================
// Protocol for DataLogger inheriters:
//   1. Build timestamped outputDir under
//      TESTS/PROCESSORS/<PROCESSOR>/OUTPUT/<TEST CASE NAME>/<timestamp>.
//      Treat outputDir as the root directory for the logger.
//   2. Per SECTION, configure the logger then call startLogging():
//        processor.setDataLogRootDirectory(outputDir);
//        processor.setDataLogOutputName("<section name>");
//        processor.startLogging();
//   3. Run processBlock one or more times. Each call appends 2 + numChannels
//      rows to input_samples.csv (pre) and output_samples.csv (post):
//      [global indices, local indices, ch0 values, ch1 values, ...].
//      After N blocks, each file has (2 + numChannels) * N rows.
//      Call stopLogging() when done.

TEST_CASE("GainProcessor applies gain and writes DataLogger output", "[GainProcessor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    GainProcessor processor;

    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/GAIN_PROCESSOR/OUTPUT/GainProcessor applies gain and writes DataLogger output")
                               .getChildFile (timestamp);

    const int numChannels = 2;
    const int numSamples  = 256;

    auto runGainSection = [&] (float gain, const juce::String& sectionName)
    {
        processor.setDataLogRootDirectory (outputDir);
        processor.setDataLogOutputName (sectionName);
        processor.startLogging();

        processor.setGain (gain);

        const int numBlocks = 2;
        juce::MidiBuffer midi;

        for (int b = 0; b < numBlocks; ++b)
        {
            juce::AudioBuffer<float> buffer (numChannels, numSamples);
            BufferFiller::fillWithAllOnes (buffer);
            processor.processBlock (buffer, midi);

            for (int ch = 0; ch < numChannels; ++ch)
                for (int s = 0; s < numSamples; ++s)
                    REQUIRE (buffer.getSample (ch, s) == Catch::Approx (gain).margin (1e-6));
        }

        auto sectionDir = outputDir.getChildFile (sectionName);
        auto inFile  = sectionDir.getChildFile ("input_samples.csv");
        auto outFile = sectionDir.getChildFile ("output_samples.csv");
        REQUIRE (inFile .existsAsFile());
        REQUIRE (outFile.existsAsFile());

        auto countLines = [] (const juce::File& f)
        {
            return juce::StringArray::fromLines (f.loadFileAsString().trimEnd()).size();
        };
        const int rowsPerBlock = 2 + numChannels; // global + local + per-channel
        REQUIRE (countLines (inFile)  == rowsPerBlock * numBlocks);
        REQUIRE (countLines (outFile) == rowsPerBlock * numBlocks);

        auto stateLog = processor.createProcessorDataLogFile();
        REQUIRE (stateLog.existsAsFile());

        processor.stopLogging();
    };

    SECTION("Gain 1.0 leaves all-ones buffer unchanged")
    {
        runGainSection (1.0f, "gain-1.0");
    }

    SECTION("Gain 0.5 halves all-ones buffer")
    {
        runGainSection (0.5f, "gain-0.5");
    }
}
