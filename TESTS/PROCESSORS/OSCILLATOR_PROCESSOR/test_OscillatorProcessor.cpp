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

    processor.prepareToPlay(sampleRate, blockSize);
    oscillator.prepare(sampleRate, blockSize);

    // not actually used but needed for call
    juce::MidiBuffer midiBuffer;

    // process through processor on its buffer
    processor.doProcessBlock(processorBuffer, midiBuffer);

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