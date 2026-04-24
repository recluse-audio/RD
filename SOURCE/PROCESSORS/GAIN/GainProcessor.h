/**
 * @file GainProcessor.h
 * @author Ryan Devens
 * @brief
 * @version 0.1
 * @date 2025-02-26
 *
 * @copyright Copyright (c) 2025
 *
 */


#pragma once
#include "Util/Juce_Header.h"
#include "PROCESSORS/BASE/RD_Processor.h"

/**
 * @brief Processor with one gain param and an apvts.
 *
 * Useful for gain adjustment, but also for testing and mocking.
 *
 */
class GainProcessor : public RD_Processor
                      , public juce::AudioProcessorValueTreeState::Listener
{
public:
    GainProcessor();
    ~GainProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                        { return true;   }

    const juce::String getName() const override            { return "Gain Processor"; }

    juce::AudioProcessorValueTreeState& getAPVTS();

   // AudioProcessorValueTreeState::Listener callback
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setGain(float newGain);

private:
    juce::Atomic<float> mGainValue;
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout _createParameterLayout();
    void _updateGainValue(float newValue);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainProcessor)

};
