/**
 * Created on 2026-05-08 by Ryan Devens with Peace and Love
 */

#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/BASE/RD_Processor.h"
#include <memory>

namespace rd_dsp { class Oscillator; }

class OscillatorProcessor : public RD_Processor
{
public:
    OscillatorProcessor();
    ~OscillatorProcessor() override;

    void doPrepareToPlay(double sampleRate, int maxBlockSize) override;
    void doProcessBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;


    const juce::String getName() const override{ return "Oscillator Processor"; }
   
    void setRunning(bool shouldRun);
    void setFrequency(float freq);
private:
    std::unique_ptr<rd_dsp::Oscillator> mOscillator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorProcessor)
};
