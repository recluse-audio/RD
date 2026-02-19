#include "TDPSOLA_Processor.h"
#include "TDPSOLA_Editor.h"

namespace TDPSOLA
{
    // Circular buffer holds this many seconds of audio.
    // Must be larger than the lookahead + longest expected grain period.
    constexpr double kCircularBufferSeconds = 2.0;

    // Lookahead is now calculated directly from the pitch detection window size
    // (PitchManagerConstants::kDefaultDetectionWindowSize = 2048 samples = 2^11)
    // This ensures proper alignment and uses a power-of-two value.

    constexpr int kMaxGrains = 64;

    static const juce::String kShiftRatioID = "shift_ratio";
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
}

TDPSOLA_Processor::~TDPSOLA_Processor()
{
    apvts.removeParameterListener (TDPSOLA::kShiftRatioID, this);
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
void TDPSOLA_Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int         numSamples = buffer.getNumSamples();
    const juce::int64 blockStart = mAbsoluteSampleCount;
    const juce::int64 blockEnd   = blockStart + numSamples;

    // 1. Store incoming audio in the circular buffer.
    mCircularBuffer.pushBuffer (buffer);

    // 2. Accumulate sample count. When a full detection window has been seen,
    //    detect pitch and generate grains over that completed window.
    //    The detection window ends at blockEnd and starts one window-length earlier.
    //    While incoming audio is in [blockStart, blockEnd), the window being
    //    detected is [blockEnd - windowSize, blockEnd) — the just-completed block.
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

    // 3. Overlap-add active grains into the output buffer.
    //    Grain write ranges already incorporate the lookahead offset, so query
    //    using the raw incoming block range — no extra offset needed here.
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

    return { params.begin(), params.end() };
}
