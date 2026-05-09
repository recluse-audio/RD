#include "OscillatorProcessor.h"

OscillatorProcessor::OscillatorProcessor()
: RD_Processor()
{
}

void OscillatorProcessor::doProcessBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer, midiMessages);
}
