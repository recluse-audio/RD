#include "RD_Processor.h"

RD_Processor::RD_Processor()
: AudioProcessor (_getDefaultBusesProperties())
{
}

RD_Processor::RD_Processor (const BusesProperties& busesProperties)
: AudioProcessor (busesProperties)
{
}

void RD_Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    mBlockSize  = samplesPerBlock;
}

//==================================
// PRIVATE
//==================================

//====================
//
juce::AudioProcessor::BusesProperties RD_Processor::_getDefaultBusesProperties()
{
    return BusesProperties()
                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                .withOutput ("Output", juce::AudioChannelSet::stereo(), true);
}
