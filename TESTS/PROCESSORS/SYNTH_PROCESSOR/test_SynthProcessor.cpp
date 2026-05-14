#include <catch2/catch_test_macros.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/SYNTH/SynthProcessor.h"
#include "SYNTH/Synth.h"

TEST_CASE("SynthProcessor reports its name", "[SynthProcessor]")
{
    TestUtils::SetupAndTeardown setup;
    SynthProcessor processor;

    REQUIRE(processor.getName() == "Synth Processor");
}

TEST_CASE("SynthProcessor exposes APVTS with wave position param", "[SynthProcessor][APVTS]")
{
    TestUtils::SetupAndTeardown setup;
    SynthProcessor processor;
    auto& apvts = processor.getAPVTS();

    SECTION("Parameter exists with expected ID")
    {
        REQUIRE(apvts.getParameter(SynthParams::kWavePositionID) != nullptr);
    }

    SECTION("Parameter type, name, range, default")
    {
        auto* wavePos = dynamic_cast<juce::AudioParameterFloat*> (
                            apvts.getParameter(SynthParams::kWavePositionID));
        REQUIRE(wavePos != nullptr);

        CHECK(wavePos->get() == 0.f);
        CHECK(wavePos->getNormalisableRange().start == 0.f);
        CHECK(wavePos->getNormalisableRange().end   == 1.f);
        CHECK(apvts.getParameter(SynthParams::kWavePositionID)->getName(64)
              == juce::String("Wave Position"));
    }
}

TEST_CASE("SynthProcessor processBlock runs without midi", "[SynthProcessor]")
{
    TestUtils::SetupAndTeardown setup;
    SynthProcessor processor;

    const int    blockSize   = 512;
    const int    numChannels = 2;
    const double sampleRate  = 48000;

    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;
    buffer.clear();

    REQUIRE_NOTHROW(processor.processBlock(buffer, midiBuffer));

    // No wavetable loaded + no notes -> silence expected.
    CHECK(buffer.getRMSLevel(0, 0, blockSize) == 0.f);
}

TEST_CASE("SynthProcessor accepts note on/off via MidiMessageCollector", "[SynthProcessor][MIDI]")
{
    TestUtils::SetupAndTeardown setup;
    SynthProcessor processor;

    const int    blockSize   = 512;
    const int    numChannels = 2;
    const double sampleRate  = 48000;

    processor.prepareToPlay(sampleRate, blockSize);

    auto onMsg  = juce::MidiMessage::noteOn  (1, 60, (juce::uint8) 100);
    auto offMsg = juce::MidiMessage::noteOff (1, 60, (juce::uint8) 0);
    onMsg .setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    offMsg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);

    processor.getMidiCollector().addMessageToQueue (onMsg);
    processor.getMidiCollector().addMessageToQueue (offMsg);

    juce::AudioBuffer<float> buffer(numChannels, blockSize);
    juce::MidiBuffer midiBuffer;
    buffer.clear();

    REQUIRE_NOTHROW(processor.processBlock(buffer, midiBuffer));

    // After draining, the collector-fed messages should be in the buffer.
    CHECK_FALSE(midiBuffer.isEmpty());
}
