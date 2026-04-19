#include "GrainShifterProcessor.h"
#include "EDITORS/GrainShifterEditor.h"

namespace GrainShifter
{
    // Circular buffer holds this many seconds of audio.
    // Must be larger than the lookahead + longest expected grain period.
    constexpr double kCircularBufferSeconds = 2.0;

    // Lookahead is calculated directly from the pitch detection window size
    // (PitchManagerConstants::kDefaultDetectionWindowSize = 2048 samples = 2^11)
    // This ensures proper alignment and uses a power-of-two value.

    constexpr int kMaxGrains = 64;

    static const juce::String kShiftRatioID       = "shift_ratio";
    static const juce::String kThresholdID        = "pitch_threshold";
    static const juce::String kPitchWindowSizeID  = "pitch_window_size";
    static const juce::String kPitchHopSizeID     = "pitch_hop_size";

    static const juce::StringArray kPitchWindowSizeChoices { "512", "1024", "2048", "4096", "8192" };
    static const juce::StringArray kPitchHopSizeChoices    { "256", "512", "1024", "2048", "4096" };

    static constexpr int kPitchWindowSizeDefaultIndex = 2; // "2048"
    static constexpr int kPitchHopSizeDefaultIndex    = 3; // "2048"

    // Lowest pitch we expect the detector to track. Lookahead / max grain length
    // is derived from this: one period at kLowestDetectableHz, doubled, then rounded
    // up to the next power of two.
    static constexpr double kLowestDetectableHz = 40.0;

    static int calcLookaheadSamples (double sampleRate)
    {
        const double periodSamples = sampleRate / kLowestDetectableHz;
        const double minNeeded     = periodSamples * 2.0;
        int p = 1;
        while (static_cast<double> (p) < minNeeded)
            p <<= 1;
        return p;
    }
}

//==============================================================================
GrainShifterProcessor::GrainShifterProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , mGranulator (mCircularBuffer)
    , apvts (*this, nullptr, "Parameters", _createParameterLayout())
{
    apvts.addParameterListener (GrainShifter::kShiftRatioID,      this);
    apvts.addParameterListener (GrainShifter::kThresholdID,       this);
    apvts.addParameterListener (GrainShifter::kPitchWindowSizeID, this);
    apvts.addParameterListener (GrainShifter::kPitchHopSizeID,    this);
}

GrainShifterProcessor::~GrainShifterProcessor()
{
    apvts.removeParameterListener (GrainShifter::kShiftRatioID,      this);
    apvts.removeParameterListener (GrainShifter::kThresholdID,       this);
    apvts.removeParameterListener (GrainShifter::kPitchWindowSizeID, this);
    apvts.removeParameterListener (GrainShifter::kPitchHopSizeID,    this);
}

//==============================================================================
void GrainShifterProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate           = sampleRate;
    mBlockSize            = samplesPerBlock;
    mAbsoluteSampleCount  = 0;
    mDetectionSampleCount = 0;

    const int  numChannels     = getTotalNumInputChannels();
    const int  circularBufSize = static_cast<int> (sampleRate * GrainShifter::kCircularBufferSeconds);

    // Pull initial window / hop sizes from APVTS so they match user state.
    const int initialWindowSize = GrainShifter::kPitchWindowSizeChoices
                                    [static_cast<int> (*apvts.getRawParameterValue (GrainShifter::kPitchWindowSizeID))]
                                    .getIntValue();
    const int initialHopSize    = GrainShifter::kPitchHopSizeChoices
                                    [static_cast<int> (*apvts.getRawParameterValue (GrainShifter::kPitchHopSizeID))]
                                    .getIntValue();

    const auto lookaheadSamples = static_cast<juce::int64> (GrainShifter::calcLookaheadSamples (sampleRate));

    mCircularBuffer.setSize (numChannels, circularBufSize);
    mCircularBuffer.clear();
    mPitchManager.prepare   (sampleRate, numChannels, initialWindowSize);
    mPitchManager.setHopSize (initialHopSize);
    mGranulator.prepare     (sampleRate, numChannels, lookaheadSamples, GrainShifter::kMaxGrains);
}

//==============================================================================
void GrainShifterProcessor::releaseResources()
{
    mPitchManager.reset();
    mGranulator.reset();
    mAbsoluteSampleCount  = 0;
    mDetectionSampleCount = 0;
}

//==============================================================================
bool GrainShifterProcessor::_didTransportJustStop()
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

void GrainShifterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    if (_didTransportJustStop())
        mCircularBuffer.clear();

    const int         numSamples = buffer.getNumSamples();
    const juce::int64 blockStart = mAbsoluteSampleCount;
    const juce::int64 blockEnd   = blockStart + numSamples;

    // 1. Store incoming audio in the circular buffer.
    mCircularBuffer.pushBuffer (buffer);

    mDetectionSampleCount += numSamples;

    const int currentHopSize    = mPitchManager.getHopSize();
    const int currentWindowSize = mPitchManager.getDetectionWindowSize();

    if (mDetectionSampleCount >= currentHopSize)
    {
        const juce::int64 detectionWindowEnd   = blockEnd;
        const juce::int64 detectionWindowStart = detectionWindowEnd - currentWindowSize;

        [[maybe_unused]] float detectedPeriod = mPitchManager.detect (mCircularBuffer, detectionWindowStart, mShiftRatio.get());

        auto synthMarks = mPitchManager.getSynthMarksInRange (juce::Range<juce::int64> (detectionWindowStart, detectionWindowEnd));

        if (!synthMarks.empty())
            mGranulator.generateGrains (synthMarks);

        mDetectionSampleCount -= currentHopSize;
    }

    buffer.clear();
    mGranulator.process (buffer, blockStart, blockEnd);

    mAbsoluteSampleCount += numSamples;
}

//==============================================================================
juce::AudioProcessorEditor* GrainShifterProcessor::createEditor()
{
    return new GrainShifterEditor (*this);
}

//==============================================================================
bool GrainShifterProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void GrainShifterProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == GrainShifter::kShiftRatioID)
        mShiftRatio.set (newValue);
    else if (parameterID == GrainShifter::kThresholdID)
        mPitchManager.getPitchDetector().setThreshold (newValue);
    else if (parameterID == GrainShifter::kPitchWindowSizeID)
    {
        const int idx = juce::jlimit (0, GrainShifter::kPitchWindowSizeChoices.size() - 1,
                                      static_cast<int> (newValue));
        mPitchManager.setDetectionWindowSize (GrainShifter::kPitchWindowSizeChoices[idx].getIntValue());
    }
    else if (parameterID == GrainShifter::kPitchHopSizeID)
    {
        const int idx = juce::jlimit (0, GrainShifter::kPitchHopSizeChoices.size() - 1,
                                      static_cast<int> (newValue));
        mPitchManager.setHopSize (GrainShifter::kPitchHopSizeChoices[idx].getIntValue());
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout GrainShifterProcessor::_createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        GrainShifter::kShiftRatioID,
        "Shift Ratio",
        juce::NormalisableRange<float> (0.5f, 2.0f, 0.01f),
        1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        GrainShifter::kThresholdID,
        "Pitch Threshold",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        FFT_PitchDetectorConstants::kDefaultThreshold));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        GrainShifter::kPitchWindowSizeID,
        "Pitch Window Size",
        GrainShifter::kPitchWindowSizeChoices,
        GrainShifter::kPitchWindowSizeDefaultIndex));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        GrainShifter::kPitchHopSizeID,
        "Pitch Hop Size",
        GrainShifter::kPitchHopSizeChoices,
        GrainShifter::kPitchHopSizeDefaultIndex));

    return { params.begin(), params.end() };
}
