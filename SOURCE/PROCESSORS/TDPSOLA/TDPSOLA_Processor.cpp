#include "TDPSOLA_Processor.h"
#include "EDITORS/TDPSOLA_Editor.h"

namespace TDPSOLA
{
    // Circular buffer holds this many seconds of audio.
    // Must be larger than the lookahead + longest expected grain period.
    constexpr double kCircularBufferSeconds = 2.0;

    // Lookahead is now calculated directly from the pitch detection window size
    // (PitchManagerConstants::kDefaultDetectionWindowSize = 2048 samples = 2^11)
    // This ensures proper alignment and uses a power-of-two value.

    constexpr int kMaxGrains = 64;

    static const juce::String kShiftRatioID  = "shift_ratio";
    static const juce::String kThresholdID   = "pitch_threshold";
}

//==============================================================================
TDPSOLA_Processor::TDPSOLA_Processor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , mGranulator (mCircularBuffer)
    , apvts (*this, nullptr, "Parameters", _createParameterLayout())
{
    apvts.addParameterListener (TDPSOLA::kShiftRatioID, this);
    apvts.addParameterListener (TDPSOLA::kThresholdID,  this);
}

TDPSOLA_Processor::~TDPSOLA_Processor()
{
    apvts.removeParameterListener (TDPSOLA::kShiftRatioID, this);
    apvts.removeParameterListener (TDPSOLA::kThresholdID,  this);
}

//==============================================================================
void TDPSOLA_Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate            = sampleRate;
    mBlockSize             = samplesPerBlock;
    mAbsoluteSampleCount   = 0;
    mDetectionSampleCount  = 0;

    const int  numChannels      = getTotalNumInputChannels();
    const int  circularBufSize  = static_cast<int> (sampleRate * TDPSOLA::kCircularBufferSeconds);

    // Use the detection window size from PitchManager as lookahead (power of two: 2048 = 2^11)
    const auto lookaheadSamples = static_cast<juce::int64> (PitchManagerConstants::kDefaultDetectionWindowSize);

    mCircularBuffer.setSize (numChannels, circularBufSize);
    mCircularBuffer.clear();
    mPitchManager.prepare   (sampleRate, numChannels);
    mGranulator.prepare     (sampleRate, numChannels, lookaheadSamples, TDPSOLA::kMaxGrains);
}

//==============================================================================
void TDPSOLA_Processor::releaseResources()
{
    mPitchManager.reset();
    mGranulator.reset();
    mAbsoluteSampleCount  = 0;
    mDetectionSampleCount = 0;
}

//==============================================================================
bool TDPSOLA_Processor::_didTransportJustStop()
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

void TDPSOLA_Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    if (_didTransportJustStop())
        mCircularBuffer.clear();

    const int         numSamples = buffer.getNumSamples();
    const juce::int64 blockStart = mAbsoluteSampleCount;
    const juce::int64 blockEnd   = blockStart + numSamples;

    // 1. Store incoming audio in the circular buffer.
    mCircularBuffer.pushBuffer (buffer);

    mDetectionSampleCount += numSamples;

    if (mDetectionSampleCount >= PitchManagerConstants::kDefaultDetectionWindowSize)
    {
        const juce::int64 detectionWindowEnd   = blockEnd;
        const juce::int64 detectionWindowStart = detectionWindowEnd - PitchManagerConstants::kDefaultDetectionWindowSize;

        [[maybe_unused]] float detectedPeriod = mPitchManager.detect (mCircularBuffer, detectionWindowStart, mShiftRatio.get());

        auto synthMarks = mPitchManager.getSynthMarksInRange ( juce::Range<juce::int64> (detectionWindowStart, detectionWindowEnd));

        if (!synthMarks.empty())
            mGranulator.generateGrains (synthMarks);

        mDetectionSampleCount -= PitchManagerConstants::kDefaultDetectionWindowSize;
    }

    buffer.clear();
    mGranulator.process (buffer, blockStart, blockEnd);

    mAbsoluteSampleCount += numSamples;
}

//==============================================================================
juce::AudioProcessorEditor* TDPSOLA_Processor::createEditor()
{
    return new TDPSOLA_Editor (*this);
}

//==============================================================================
bool TDPSOLA_Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void TDPSOLA_Processor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == TDPSOLA::kShiftRatioID)
        mShiftRatio.set (newValue);
    else if (parameterID == TDPSOLA::kThresholdID)
        mPitchManager.getPitchDetector().setThreshold (newValue);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TDPSOLA_Processor::_createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        TDPSOLA::kShiftRatioID,
        "Shift Ratio",
        juce::NormalisableRange<float> (0.5f, 2.0f, 0.01f),
        1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        TDPSOLA::kThresholdID,
        "Pitch Threshold",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        TD_PitchDetectorConstants::kDefaultThreshold));

    return { params.begin(), params.end() };
}
