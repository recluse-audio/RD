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
 * All gain/APVTS machinery lives in RD_Processor. GainProcessor just supplies
 * the editor and a custom name.
 */
class GainProcessor : public RD_Processor
{
public:
    GainProcessor();
    ~GainProcessor() override = default;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                        { return true;   }

    const juce::String getName() const override            { return "Gain Processor"; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainProcessor)
};
