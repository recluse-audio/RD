#include "GainProcessor.h"
#include "EDITORS/GainEditor.h"

GainProcessor::GainProcessor()
: RD_Processor()
{
}

void GainProcessor::doProcessBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    const auto gainValue = mGainValue.get();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            const auto inputSample  = buffer.getSample (ch, sampleIndex);
            const auto outputSample = inputSample * gainValue;
            buffer.setSample (ch, sampleIndex, outputSample);
        }
    }
}

juce::AudioProcessorEditor* GainProcessor::createEditor()
{
    return new GainEditor (*this);
}
