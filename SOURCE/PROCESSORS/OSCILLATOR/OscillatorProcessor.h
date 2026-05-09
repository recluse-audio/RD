/**
 * Created on 2026-05-08 by Ryan Devens with Peace and Love
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/BASE/RD_Processor.h"

class OscillatorProcessor : public RD_Processor
{
public:
    OscillatorProcessor();
    ~OscillatorProcessor() override = default;

    void doProcessBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    const juce::String getName() const override            { return "Oscillator Processor"; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorProcessor)
};
