#include "OscillatorProcessor.h"
#include "OSCILLATOR/Oscillator.h"

OscillatorProcessor::OscillatorProcessor()
: RD_Processor(),
  mOscillator (std::make_unique<rd_dsp::Oscillator>())
{
}

OscillatorProcessor::~OscillatorProcessor() = default;

void OscillatorProcessor::doPrepareToPlay(double sampleRate, int maxBlockSize)
{
    mOscillator->prepare(sampleRate, maxBlockSize);
}

void OscillatorProcessor::doProcessBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer, midiMessages);
    auto* readPtr = buffer.getArrayOfReadPointers();
    auto* writePtr = buffer.getArrayOfWritePointers();
    mOscillator->process(readPtr, writePtr, buffer.getNumChannels(), buffer.getNumSamples());
}

void OscillatorProcessor::setRunning(bool shouldRun)
{
    if(shouldRun)
        mOscillator->start();
    else
        mOscillator->stop();
}

void OscillatorProcessor::setFrequency(float freq)
{
    mOscillator->setFreq(freq);
}