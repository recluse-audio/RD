#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../TEST_UTILS/BufferGenerator.h"
#include "../../../SOURCE/PROCESSORS/GAIN/GainProcessor.h"



TEST_CASE("GainProcessor applies gain exactly to unity input", "[GainProcessor]")
{
    TestUtils::SetupAndTeardown setup;
    GainProcessor processor;

    const double sampleRate = 44100.0;
    const int samplesPerBlock = 512;
    const int numChannels = 2;

    processor.prepareToPlay(sampleRate, samplesPerBlock);

    SECTION("Gain of 0.0 produces exactly zero output")
    {
        processor.setGain(0.0f);

        juce::AudioBuffer<float> buffer(numChannels, samplesPerBlock);
        BufferGenerator::fillWithAllOnes(buffer);

        juce::MidiBuffer midiBuffer;
        processor.processBlock(buffer, midiBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < samplesPerBlock; ++i)
            {
                REQUIRE(buffer.getSample(ch, i) == 0.0f);
            }
        }
    }

    SECTION("Gain of 0.25 produces exactly 0.25 output")
    {
        processor.setGain(0.25f);

        juce::AudioBuffer<float> buffer(numChannels, samplesPerBlock);
        BufferGenerator::fillWithAllOnes(buffer);

        juce::MidiBuffer midiBuffer;
        processor.processBlock(buffer, midiBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < samplesPerBlock; ++i)
            {
                REQUIRE_THAT(buffer.getSample(ch, i),
                    Catch::Matchers::WithinAbs(0.25f, 1e-7f));
            }
        }
    }

    SECTION("Gain of 0.5 produces exactly 0.5 output")
    {
        processor.setGain(0.5f);

        juce::AudioBuffer<float> buffer(numChannels, samplesPerBlock);
        BufferGenerator::fillWithAllOnes(buffer);

        juce::MidiBuffer midiBuffer;
        processor.processBlock(buffer, midiBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < samplesPerBlock; ++i)
            {
                REQUIRE_THAT(buffer.getSample(ch, i),
                    Catch::Matchers::WithinAbs(0.5f, 1e-7f));
            }
        }
    }

    SECTION("Gain of 0.75 produces exactly 0.75 output")
    {
        processor.setGain(0.75f);

        juce::AudioBuffer<float> buffer(numChannels, samplesPerBlock);
        BufferGenerator::fillWithAllOnes(buffer);

        juce::MidiBuffer midiBuffer;
        processor.processBlock(buffer, midiBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < samplesPerBlock; ++i)
            {
                REQUIRE_THAT(buffer.getSample(ch, i),
                    Catch::Matchers::WithinAbs(0.75f, 1e-7f));
            }
        }
    }

    SECTION("Gain of 1.0 produces exactly 1.0 output (unity gain)")
    {
        processor.setGain(1.0f);

        juce::AudioBuffer<float> buffer(numChannels, samplesPerBlock);
        BufferGenerator::fillWithAllOnes(buffer);

        juce::MidiBuffer midiBuffer;
        processor.processBlock(buffer, midiBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < samplesPerBlock; ++i)
            {
                REQUIRE_THAT(buffer.getSample(ch, i),
                    Catch::Matchers::WithinAbs(1.0f, 1e-7f));
            }
        }
    }

    SECTION("Arbitrary gain value applies exactly")
    {
        const float testGain = 0.12345f;
        processor.setGain(testGain);

        juce::AudioBuffer<float> buffer(numChannels, samplesPerBlock);
        BufferGenerator::fillWithAllOnes(buffer);

        juce::MidiBuffer midiBuffer;
        processor.processBlock(buffer, midiBuffer);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < samplesPerBlock; ++i)
            {
                REQUIRE_THAT(buffer.getSample(ch, i),
                    Catch::Matchers::WithinAbs(testGain, 1e-7f));
            }
        }
    }

    processor.releaseResources();
}

TEST_CASE("GainProcessor maintains gain consistency across multiple blocks", "[GainProcessor]")
{
    TestUtils::SetupAndTeardown setup;
    GainProcessor processor;

    const double sampleRate = 44100.0;
    const int samplesPerBlock = 256;
    const int numChannels = 2;
    const float testGain = 0.6789f;

    processor.prepareToPlay(sampleRate, samplesPerBlock);
    processor.setGain(testGain);

    SECTION("Gain remains consistent across 10 consecutive blocks")
    {
        for (int block = 0; block < 10; ++block)
        {
            juce::AudioBuffer<float> buffer(numChannels, samplesPerBlock);
            BufferGenerator::fillWithAllOnes(buffer);

            juce::MidiBuffer midiBuffer;
            processor.processBlock(buffer, midiBuffer);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                for (int i = 0; i < samplesPerBlock; ++i)
                {
                    REQUIRE_THAT(buffer.getSample(ch, i),
                        Catch::Matchers::WithinAbs(testGain, 1e-7f));
                }
            }
        }
    }

    processor.releaseResources();
}