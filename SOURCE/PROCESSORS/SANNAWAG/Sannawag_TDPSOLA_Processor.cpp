#include "Sannawag_TDPSOLA_Processor.h"

namespace Sannawag
{
    static const juce::String kShiftRatioID = "shift_ratio";
}

//==============================================================================
Sannawag_TDPSOLA_Processor::Sannawag_TDPSOLA_Processor()
    : RD_Processor()
    , mAPVTS (*this, nullptr, "Parameters", createParameterLayout())
{
    mAPVTS.addParameterListener (Sannawag::kShiftRatioID, this);
}

Sannawag_TDPSOLA_Processor::~Sannawag_TDPSOLA_Processor()
{
    mAPVTS.removeParameterListener (Sannawag::kShiftRatioID, this);
}

//==============================================================================
void Sannawag_TDPSOLA_Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    RD_Processor::prepareToPlay (sampleRate, samplesPerBlock);
    mAbsoluteSampleCount = 0;

    mScratchBuffer.setSize (getTotalNumInputChannels(), samplesPerBlock, false, true, false);
    mScratchBuffer.clear();
}

//==============================================================================
void Sannawag_TDPSOLA_Processor::releaseResources()
{
    mAbsoluteSampleCount = 0;
    mScratchBuffer.setSize (0, 0);
}

//==============================================================================
bool Sannawag_TDPSOLA_Processor::_didTransportJustStop()
{
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            const bool isPlaying = pos->getIsPlaying();
            const bool stopped   = mWasPlaying && ! isPlaying;
            mWasPlaying = isPlaying;
            return stopped;
        }
    }
    return false;
}

void Sannawag_TDPSOLA_Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ignoreUnused (_didTransportJustStop());

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    if (mScratchBuffer.getNumChannels() < numChannels || mScratchBuffer.getNumSamples() < numSamples)
        mScratchBuffer.setSize (numChannels, numSamples, false, true, false);

    const bool ok = mEngine.process (buffer, mScratchBuffer, mShiftRatio.get(),
                                     static_cast<float> (mSampleRate), mConfig);

    if (ok)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.copyFrom (ch, 0, mScratchBuffer, ch, 0, numSamples);
    }
    // On failure, leave the input pass-through in `buffer`.

    mAbsoluteSampleCount += numSamples;
}

//==============================================================================
bool Sannawag_TDPSOLA_Processor::renderOffline (const juce::AudioBuffer<float>& input,
                                                juce::AudioBuffer<float>&       output,
                                                float                           shiftRatio)
{
    return mEngine.process (input, output, shiftRatio,
                            static_cast<float> (mSampleRate), mConfig);
}

//==============================================================================
juce::AudioProcessorEditor* Sannawag_TDPSOLA_Processor::createEditor()
{
    return nullptr;
}

//==============================================================================
bool Sannawag_TDPSOLA_Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void Sannawag_TDPSOLA_Processor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == Sannawag::kShiftRatioID)
        mShiftRatio.set (newValue);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout Sannawag_TDPSOLA_Processor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        Sannawag::kShiftRatioID,
        "Shift Ratio",
        juce::NormalisableRange<float> (0.5f, 2.0f, 0.01f),
        1.0f));

    return { params.begin(), params.end() };
}
