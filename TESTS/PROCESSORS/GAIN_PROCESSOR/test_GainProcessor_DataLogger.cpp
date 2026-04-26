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
//   2. Call processor.createOutputDirectory(outputDir) once per test case.
//   3. Per SECTION, create a sectionDir under outputDir, then
//      setOutputFile(sectionDir) so each section's logs are isolated.
//   4. Log pre-process buffer, run processBlock, log post-process buffer,
//      then log processor state. REQUIRE each returned juce::File exists.

TEST_CASE("GainProcessor applies gain and writes DataLogger output", "[GainProcessor][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    GainProcessor processor;

    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File outputDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/GAIN_PROCESSOR/OUTPUT/GainProcessor applies gain and writes DataLogger output")
                               .getChildFile (timestamp);
    processor.createOutputDirectory (outputDir);

    const int numChannels = 2;
    const int numSamples  = 256;

    auto runGainSection = [&] (float gain, const juce::String& sectionName)
    {
        auto sectionDir = outputDir.getChildFile (sectionName);
        processor.createOutputDirectory (sectionDir);
        processor.setOutputFile (sectionDir);

        processor.setGain (gain);

        juce::AudioBuffer<float> buffer (numChannels, numSamples);
        BufferFiller::fillWithAllOnes (buffer);

        auto preLog = processor.createProcessBlockDataLogFile (buffer, true);
        REQUIRE(preLog.existsAsFile());

        juce::MidiBuffer midi;
        processor.processBlock (buffer, midi);

        auto postLog = processor.createProcessBlockDataLogFile (buffer, false);
        REQUIRE(postLog.existsAsFile());

        auto stateLog = processor.createProcessorDataLogFile();
        REQUIRE(stateLog.existsAsFile());

        for (int ch = 0; ch < numChannels; ++ch)
            for (int s = 0; s < numSamples; ++s)
                REQUIRE(buffer.getSample (ch, s) == Catch::Approx (gain).margin (1e-6));
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
