#include <catch2/catch_test_macros.hpp>
#include "../../TEST_UTILS/TestUtils.h"
#include "../../../SOURCE/PROCESSORS/OSCILLATOR/OscillatorProcessor.h"
#include "OSCILLATOR/Oscillator.h"

TEST_CASE("OscillatorProcessor reports its name", "[OscillatorProcessor]")
{
    TestUtils::SetupAndTeardown setup;
    OscillatorProcessor processor;

    REQUIRE(processor.getName() == "Oscillator Processor");
}

TEST_CASE("OscillatorProcessor processes sine wave", "[OscillatorProcessor]")
{
    TestUtils::SetupAndTeardown setup;
    OscillatorProcessor processor;
    rd_dsp::Oscillator oscillator;

    int blockSize = 512;
    int numChannels = 2;
    double sampleRate = 48000;

    // use these buffers, get their read/write ptrs process with processor
    // and with oscillator independently and compare their results
    juce::AudioBuffer<float> processorBuffer(numChannels, blockSize);
    juce::AudioBuffer<float> oscillatorBuffer(numChannels, blockSize);

    processorBuffer.clear();
    oscillatorBuffer.clear();

    processor.prepareToPlay(sampleRate, blockSize);
    oscillator.prepare(sampleRate, blockSize);

    float freq = 440.f;

    // Drive processor exclusively through its APVTS — parameterChanged is the
    // only path that should update the internal oscillator.
    auto& apvts = processor.getAPVTS();
    *dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter(OscillatorParams::kFrequencyID)) = freq;
    *dynamic_cast<juce::AudioParameterBool*>  (apvts.getParameter(OscillatorParams::kOnOffID))     = true;

    // APVTS listener callbacks dispatch via AsyncUpdater — pump message loop
    // so parameterChanged() fires before processBlock runs.
    juce::MessageManager::getInstance()->runDispatchLoopUntil(50);

    oscillator.setFreq(freq);
    oscillator.start();

    // not actually used but needed for call
    juce::MidiBuffer midiBuffer;

    // process through processor on its buffer
    processor.processBlock(processorBuffer, midiBuffer);

    // get as write/read ptr and process through oscillator directly
    auto* readPtr = oscillatorBuffer.getArrayOfReadPointers();
    auto* writePtr = oscillatorBuffer.getArrayOfWritePointers();
    oscillator.process(readPtr, writePtr, numChannels, blockSize);

    REQUIRE(processorBuffer.getRMSLevel(0, 0, blockSize) > 0.f);
    REQUIRE(oscillatorBuffer.getRMSLevel(0, 0, blockSize) > 0.f);

    for(int sampleIndex = 0; sampleIndex < blockSize; sampleIndex++)
    {
        for(int ch = 0; ch < numChannels; ch++)
        {
            float processSample = processorBuffer.getSample(ch, sampleIndex);
            float oscSample = oscillatorBuffer.getSample(ch, sampleIndex);
            CHECK(processSample == oscSample);
        }
    }

}

TEST_CASE("OscillatorProcessor exposes APVTS with on/off and frequency params", "[OscillatorProcessor][APVTS]")
{
    TestUtils::SetupAndTeardown setup;
    OscillatorProcessor processor;
    auto& apvts = processor.getAPVTS();

    SECTION("Parameters exist with expected IDs")
    {
        REQUIRE(apvts.getParameter(OscillatorParams::kOnOffID)     != nullptr);
        REQUIRE(apvts.getParameter(OscillatorParams::kFrequencyID) != nullptr);
    }

    SECTION("Parameter types and defaults")
    {
        auto* onOff = dynamic_cast<juce::AudioParameterBool*>  (apvts.getParameter(OscillatorParams::kOnOffID));
        auto* freq  = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter(OscillatorParams::kFrequencyID));

        REQUIRE(onOff != nullptr);
        REQUIRE(freq  != nullptr);

        CHECK(onOff->get() == false);
        CHECK(freq->get()  == 440.f);

        CHECK(freq->getNormalisableRange().start == 20.f);
        CHECK(freq->getNormalisableRange().end   == 20000.f);
    }

    SECTION("Parameter names match spec")
    {
        CHECK(apvts.getParameter(OscillatorParams::kOnOffID)->getName(64)     == juce::String("Oscillator On/Off"));
        CHECK(apvts.getParameter(OscillatorParams::kFrequencyID)->getName(64) == juce::String("Oscillator Frequency"));
    }
}

TEST_CASE("OscillatorProcessor APVTS drives oscillator state", "[OscillatorProcessor][APVTS]")
{
    TestUtils::SetupAndTeardown setup;
    OscillatorProcessor processor;
    auto& apvts = processor.getAPVTS();

    const int    blockSize   = 512;
    const int    numChannels = 2;
    const double sampleRate  = 48000;

    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(numChannels, blockSize);
    juce::MidiBuffer         midiBuffer;

    auto* onOff = dynamic_cast<juce::AudioParameterBool*>  (apvts.getParameter(OscillatorParams::kOnOffID));
    auto* freq  = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter(OscillatorParams::kFrequencyID));
    REQUIRE(onOff != nullptr);
    REQUIRE(freq  != nullptr);

    SECTION("On/Off=false leaves buffer silent")
    {
        *freq  = 440.f;
        *onOff = false;
        juce::MessageManager::getInstance()->runDispatchLoopUntil(50);

        buffer.clear();
        processor.processBlock(buffer, midiBuffer);
        CHECK(buffer.getRMSLevel(0, 0, blockSize) == 0.f);
    }

    SECTION("On/Off=true produces signal")
    {
        *freq  = 440.f;
        *onOff = true;
        juce::MessageManager::getInstance()->runDispatchLoopUntil(50);

        buffer.clear();
        processor.processBlock(buffer, midiBuffer);
        CHECK(buffer.getRMSLevel(0, 0, blockSize) > 0.f);
    }

    SECTION("Different frequencies produce different output")
    {
        *onOff = true;

        *freq = 110.f;
        juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
        juce::AudioBuffer<float> lowFreqBuf(numChannels, blockSize);
        lowFreqBuf.clear();
        processor.processBlock(lowFreqBuf, midiBuffer);

        // fresh processor so phase doesn't carry over
        OscillatorProcessor processor2;
        processor2.prepareToPlay(sampleRate, blockSize);
        auto& apvts2 = processor2.getAPVTS();
        *dynamic_cast<juce::AudioParameterBool*>  (apvts2.getParameter(OscillatorParams::kOnOffID))     = true;
        *dynamic_cast<juce::AudioParameterFloat*> (apvts2.getParameter(OscillatorParams::kFrequencyID)) = 880.f;
        juce::MessageManager::getInstance()->runDispatchLoopUntil(50);

        juce::AudioBuffer<float> highFreqBuf(numChannels, blockSize);
        highFreqBuf.clear();
        processor2.processBlock(highFreqBuf, midiBuffer);

        bool anyDifferent = false;
        for (int s = 0; s < blockSize && !anyDifferent; ++s)
            if (lowFreqBuf.getSample(0, s) != highFreqBuf.getSample(0, s))
                anyDifferent = true;

        CHECK(anyDifferent);
    }
}
