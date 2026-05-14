#include <catch2/catch_test_macros.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/PULSAR/PulsarProcessor.h"
#include "PULSAR/PulsarTrain.h"

TEST_CASE("PulsarProcessor reports its name", "[PulsarProcessor]")
{
    TestUtils::SetupAndTeardown setup;
    PulsarProcessor processor;

    REQUIRE(processor.getName() == "Pulsar Processor");
}

TEST_CASE("PulsarProcessor exposes APVTS with fundamental, formant, wave position", "[PulsarProcessor][APVTS]")
{
    TestUtils::SetupAndTeardown setup;
    PulsarProcessor processor;
    auto& apvts = processor.getAPVTS();

    SECTION("Parameters exist with expected IDs")
    {
        REQUIRE(apvts.getParameter(PulsarParams::kFundamentalFreqID) != nullptr);
        REQUIRE(apvts.getParameter(PulsarParams::kFormantFreqID)     != nullptr);
        REQUIRE(apvts.getParameter(PulsarParams::kWavePositionID)    != nullptr);
    }

    SECTION("Parameter ranges come from rd_dsp::PulsarTrain constants")
    {
        auto* fund    = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PulsarParams::kFundamentalFreqID));
        auto* formant = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PulsarParams::kFormantFreqID));
        auto* wave    = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PulsarParams::kWavePositionID));

        REQUIRE(fund    != nullptr);
        REQUIRE(formant != nullptr);
        REQUIRE(wave    != nullptr);

        CHECK(fund->getNormalisableRange().start    == rd_dsp::PulsarTrain::kMinEmissionRate);
        CHECK(fund->getNormalisableRange().end      == rd_dsp::PulsarTrain::kMaxEmissionRate);
        CHECK(formant->getNormalisableRange().start == rd_dsp::PulsarTrain::kMinFormantFreq);
        CHECK(formant->getNormalisableRange().end   == rd_dsp::PulsarTrain::kMaxFormantFreq);
        CHECK(wave->getNormalisableRange().start    == 0.f);
        CHECK(wave->getNormalisableRange().end      == 1.f);
    }

    SECTION("Parameter names match spec")
    {
        CHECK(apvts.getParameter(PulsarParams::kFundamentalFreqID)->getName(64) == juce::String("Fundamental Frequency"));
        CHECK(apvts.getParameter(PulsarParams::kFormantFreqID)->getName(64)     == juce::String("Formant Frequency"));
        CHECK(apvts.getParameter(PulsarParams::kWavePositionID)->getName(64)    == juce::String("Wave Position"));
    }
}

TEST_CASE("PulsarProcessor processBlock runs without throwing after prepare", "[PulsarProcessor]")
{
    TestUtils::SetupAndTeardown setup;
    PulsarProcessor processor;

    const int    blockSize   = 512;
    const int    numChannels = 2;
    const double sampleRate  = 48000;

    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(numChannels, blockSize);
    juce::MidiBuffer         midiBuffer;
    buffer.clear();

    REQUIRE_NOTHROW(processor.processBlock(buffer, midiBuffer));
}

TEST_CASE("PulsarProcessor APVTS drives PulsarTrain state", "[PulsarProcessor][APVTS]")
{
    TestUtils::SetupAndTeardown setup;
    PulsarProcessor processor;
    auto& apvts = processor.getAPVTS();

    const int    blockSize   = 512;
    const double sampleRate  = 48000;

    processor.prepareToPlay(sampleRate, blockSize);

    auto* fund    = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PulsarParams::kFundamentalFreqID));
    auto* formant = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PulsarParams::kFormantFreqID));
    REQUIRE(fund    != nullptr);
    REQUIRE(formant != nullptr);

    // Listener callbacks dispatch via AsyncUpdater — pump message loop so
    // parameterChanged() fires before we observe state on the train.
    *fund    = 25.f;
    *formant = 880.f;
    juce::MessageManager::getInstance()->runDispatchLoopUntil(50);

    // PulsarProcessor doesn't expose its train, so we can only verify indirectly:
    // a non-trivial processBlock shouldn't crash and should produce *some* output
    // once the train has been started + driven via APVTS. Default Wavetable is
    // empty, so signal level isn't asserted here — only that the call is safe.
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer         midiBuffer;
    buffer.clear();

    REQUIRE_NOTHROW(processor.processBlock(buffer, midiBuffer));
}
