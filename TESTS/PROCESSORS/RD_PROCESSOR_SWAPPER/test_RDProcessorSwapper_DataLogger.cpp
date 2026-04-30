#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/RD_ProcessorSwapper.h"
#include "../../../SOURCE/BUFFER_FILLER/BufferFiller.h"

//========================================================
//===================== DATA LOGGING =====================
//========================================================
// Protocol for DataLogger inheriters:
//   Path layout:
//     <OUTPUT_BASE>/<TEST_NAME>[/<SECTION_NAME>]/TEST_CASE_ROOT_DIR/DATA_LOG_OUTPUT_DIR_<timestamp>
//   1. Per SECTION (or once per case), configure the logger then call startLogging():
//        processor.setDataLogRootDirectory (testDir/[sectionName]/"TEST_CASE_ROOT_DIR");
//        processor.setDataLogOutputName ("DATA_LOG_OUTPUT_DIR_" + timestamp);
//        processor.startLogging();
//   2. Run processBlock; appends happen automatically. Call stopLogging() when done.

TEST_CASE("RD_ProcessorSwapper registers every contained RD_Processor as a DataLogger child", "[RD_ProcessorSwapper][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;

    const int numProcessors = swapper.getNumProcessors();
    REQUIRE (numProcessors > 0);

    REQUIRE (swapper.getNumChildren() == static_cast<size_t> (numProcessors));

    for (int i = 0; i < numProcessors; ++i)
    {
        auto index = static_cast<RD_ProcessorSwapper::ProcessorIndex> (i);
        auto* audioProc = swapper.getProcessorByIndex (index);
        REQUIRE (audioProc != nullptr);

        auto* rdProc = dynamic_cast<RD_Processor*> (audioProc);
        REQUIRE (rdProc != nullptr);

        DataLogger* asLogger = rdProc;
        REQUIRE (asLogger->getParentLogger() == static_cast<DataLogger*> (&swapper));
    }
}

TEST_CASE("RD_ProcessorSwapper child loggers nest output directory under swapper output directory", "[RD_ProcessorSwapper][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;

    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    juce::File rootDir = juce::File (__FILE__).getParentDirectory()
                                              .getChildFile ("OUTPUT")
                                              .getChildFile ("RD_ProcessorSwapper child loggers nest under swapper output directory")
                                              .getChildFile ("TEST_CASE_ROOT_DIR");

    swapper.setDataLogRootDirectory (rootDir);
    swapper.setDataLogOutputName ("DATA_LOG_OUTPUT_DIR_" + timestamp);

    const auto swapperDir = swapper.getDataLogOutputDirectory();

    for (int i = 0; i < swapper.getNumProcessors(); ++i)
    {
        auto index = static_cast<RD_ProcessorSwapper::ProcessorIndex> (i);
        auto* audioProc = swapper.getProcessorByIndex (index);
        REQUIRE (audioProc != nullptr);
        auto* rdProc = dynamic_cast<RD_Processor*> (audioProc);
        REQUIRE (rdProc != nullptr);

        REQUIRE (rdProc->getDataLogParentDirectory() == swapperDir);
        REQUIRE (rdProc->getDataLogOutputDirectory()
                 == swapperDir.getChildFile (rdProc->getDataLogOutputName()));
    }
}

TEST_CASE("RD_ProcessorSwapper routes through GainProcessor child and nests its DataLog", "[RD_ProcessorSwapper][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;

    const double sampleRate  = 44100.0;
    const int    numChannels = 2;
    const int    numSamples  = 256;

    swapper.prepareToPlay (sampleRate, numSamples);
    swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGain);

    auto* gainProc = dynamic_cast<RD_Processor*> (
        swapper.getProcessorByIndex (RD_ProcessorSwapper::ProcessorIndex::kGain));
    REQUIRE (gainProc != nullptr);

    juce::File testDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/RD_PROCESSOR_SWAPPER/OUTPUT/RD_ProcessorSwapper routes through GainProcessor child");

    auto runGainSection = [&] (float gain, const juce::String& sectionName)
    {
        auto timestamp  = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
        auto rootDir    = testDir.getChildFile (sectionName).getChildFile ("TEST_CASE_ROOT_DIR");
        auto outputName = "DATA_LOG_OUTPUT_DIR_" + timestamp;

        swapper.setDataLogRootDirectory (rootDir);
        swapper.setDataLogOutputName (outputName);
        swapper.startLogging();
        gainProc->startLogging();

        auto* gainParam = gainProc->getAPVTS().getParameter ("gain");
        REQUIRE (gainParam != nullptr);
        gainParam->setValueNotifyingHost (gainParam->getNormalisableRange().convertTo0to1 (gain));

        juce::MidiBuffer midi;
        const int numBlocks = 2;

        for (int b = 0; b < numBlocks; ++b)
        {
            juce::AudioBuffer<float> buffer (numChannels, numSamples);
            BufferFiller::fillWithAllOnes (buffer);
            swapper.processBlock (buffer, midi);

            for (int ch = 0; ch < numChannels; ++ch)
                for (int s = 0; s < numSamples; ++s)
                    REQUIRE (buffer.getSample (ch, s) == Catch::Approx (gain).margin (1e-6));
        }

        auto swapperDir = swapper.getDataLogOutputDirectory();
        auto gainDir    = gainProc->getDataLogOutputDirectory();
        REQUIRE (gainDir.isAChildOf (swapperDir));

        for (int b = 0; b < numBlocks; ++b)
        {
            const auto idx = juce::String (static_cast<juce::int64> (b) * numSamples);
            REQUIRE (swapperDir.getChildFile ("process_block_start_" + idx).getChildFile ("input_samples.csv") .existsAsFile());
            REQUIRE (swapperDir.getChildFile ("process_block_end_"   + idx).getChildFile ("output_samples.csv").existsAsFile());
            REQUIRE (gainDir   .getChildFile ("process_block_start_" + idx).getChildFile ("input_samples.csv") .existsAsFile());
            REQUIRE (gainDir   .getChildFile ("process_block_end_"   + idx).getChildFile ("output_samples.csv").existsAsFile());
        }

        auto swapperState = swapper.createProcessorDataLogFile();
        REQUIRE (swapperState.existsAsFile());

        gainProc->stopLogging();
        swapper.stopLogging();
    };

    SECTION("Gain 1.0 leaves all-ones buffer unchanged")
    {
        runGainSection (1.0f, "gain-1.0");
    }

    SECTION("Gain 0.5 halves all-ones buffer")
    {
        runGainSection (0.5f, "gain-0.5");
    }

    swapper.releaseResources();
}

TEST_CASE("RD_ProcessorSwapper writes no CSV when global logging is disabled", "[RD_ProcessorSwapper][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;

    const double sampleRate  = 44100.0;
    const int    numChannels = 2;
    const int    numSamples  = 256;

    swapper.prepareToPlay (sampleRate, numSamples);
    swapper.setActiveProcessor (RD_ProcessorSwapper::ProcessorIndex::kGain);

    juce::File testDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/RD_PROCESSOR_SWAPPER/OUTPUT/RD_ProcessorSwapper writes no CSV when global logging is disabled");

    auto runNoLogSection = [&] (float gain, const juce::String& sectionName)
    {
        auto timestamp  = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
        auto rootDir    = testDir.getChildFile (sectionName).getChildFile ("TEST_CASE_ROOT_DIR");
        auto outputName = "DATA_LOG_OUTPUT_DIR_" + timestamp;

        swapper.setDataLogRootDirectory (rootDir);
        swapper.setDataLogOutputName (outputName);
        auto sectionDir = swapper.getDataLogOutputDirectory();

        swapper.setIsLogging (false);
        swapper.setGain (gain);

        juce::AudioBuffer<float> buffer (numChannels, numSamples);
        BufferFiller::fillWithAllOnes (buffer);

        juce::MidiBuffer midi;
        swapper.processBlock (buffer, midi);

        REQUIRE_FALSE (sectionDir.getChildFile ("process_block_start_0").getChildFile ("input_samples.csv") .existsAsFile());
        REQUIRE_FALSE (sectionDir.getChildFile ("process_block_end_0")  .getChildFile ("output_samples.csv").existsAsFile());

        for (int ch = 0; ch < numChannels; ++ch)
            for (int s = 0; s < numSamples; ++s)
                REQUIRE(buffer.getSample (ch, s) == Catch::Approx (gain).margin (1e-6));
    };

    SECTION("Gain 1.0 with logging off writes no CSV")
    {
        runNoLogSection (1.0f, "gain-1.0-nolog");
    }

    SECTION("Gain 0.5 with logging off writes no CSV")
    {
        runNoLogSection (0.5f, "gain-0.5-nolog");
    }

    swapper.releaseResources();
}

TEST_CASE("RD_ProcessorSwapper writes APVTS gain value into processor state log", "[RD_ProcessorSwapper][DataLogger]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;

    const double sampleRate  = 44100.0;
    const int    numSamples  = 256;

    swapper.prepareToPlay (sampleRate, numSamples);

    juce::File testDir = juce::File ("c:/REPOS/PLUGIN_PROJECTS/RD/TESTS/PROCESSORS/RD_PROCESSOR_SWAPPER/OUTPUT/RD_ProcessorSwapper writes APVTS gain value into processor state log");

    auto runApvtsSection = [&] (float gain, const juce::String& sectionName)
    {
        auto timestamp  = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
        auto rootDir    = testDir.getChildFile (sectionName).getChildFile ("TEST_CASE_ROOT_DIR");
        auto outputName = "DATA_LOG_OUTPUT_DIR_" + timestamp;

        swapper.setDataLogRootDirectory (rootDir);
        swapper.setDataLogOutputName (outputName);
        swapper.getDataLogOutputDirectory().createDirectory();

        auto* param = swapper.getAPVTS().getParameter ("gain");
        REQUIRE(param != nullptr);
        param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (gain));

        auto stateLog = swapper.createProcessorDataLogFile();
        REQUIRE(stateLog.existsAsFile());
        REQUIRE(stateLog.getFileExtension() == ".xml");

        auto xml = juce::XmlDocument::parse (stateLog);
        REQUIRE(xml != nullptr);
        REQUIRE(xml->getStringAttribute ("processorName") == swapper.getName());

        auto* gainElem = xml->getChildByAttribute ("id", "gain");
        REQUIRE(gainElem != nullptr);
        REQUIRE(gainElem->hasAttribute ("value"));
        REQUIRE(gainElem->getDoubleAttribute ("value") == Catch::Approx (gain).margin (1e-6));
    };

    SECTION("APVTS gain 1.0 round-trips through state log")
    {
        runApvtsSection (1.0f, "apvts-gain-1.0");
    }

    SECTION("APVTS gain 0.5 round-trips through state log")
    {
        runApvtsSection (0.5f, "apvts-gain-0.5");
    }

    swapper.releaseResources();
}
