#include "TDPSOLA_Processor.h"

namespace
{
    // Circular buffer holds this many seconds of audio.
    // Must be larger than the lookahead + longest expected grain period.
    constexpr double kCircularBufferSeconds = 2.0;

    // Lookahead in seconds. Pitch detection needs a window of audio ahead of
    // the current playback position to place accurate pitch marks.
    constexpr double kLookaheadSeconds = 0.05; // 50 ms

    constexpr int kMaxGrains = 64;
}

//==============================================================================
TDPSOLA_Processor::TDPSOLA_Processor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , mGranulator (mCircularBuffer)   // mCircularBuffer is already constructed here
{
}

TDPSOLA_Processor::~TDPSOLA_Processor() {}

//==============================================================================
void TDPSOLA_Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate           = sampleRate;
    mBlockSize            = samplesPerBlock;
    mAbsoluteSampleCount  = 0;

    const int   numChannels      = getTotalNumInputChannels();
    const int   circularBufSize  = static_cast<int> (sampleRate * kCircularBufferSeconds);
    const auto  lookaheadSamples = static_cast<juce::int64> (sampleRate * kLookaheadSeconds);

    mCircularBuffer.setSize (numChannels, circularBufSize);
    mPitchManager.prepare   (sampleRate);
    mGranulator.prepare     (sampleRate, numChannels, lookaheadSamples, kMaxGrains);
}

//==============================================================================
void TDPSOLA_Processor::releaseResources()
{
    mPitchManager.reset();
    mGranulator.reset();
    mAbsoluteSampleCount = 0;
}

//==============================================================================
void TDPSOLA_Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int       numSamples = buffer.getNumSamples();
    const juce::int64 blockStart = mAbsoluteSampleCount;
    const juce::int64 blockEnd   = blockStart + numSamples;

    // 1. Push incoming audio into the CircularBuffer so grains can read it later.
    mCircularBuffer.pushBuffer (buffer);

    // 2. Run pitch detection. PitchManager accumulates samples internally and
    //    fires detection when its window is full; pitch/synth marks are stored
    //    in absolute sample time.
    mPitchManager.process (buffer, mCircularBuffer);

    // 3. Fetch any synth marks whose centres fall inside the current block.
    auto synthMarks = mPitchManager.getSynthMarksInRange (juce::Range<juce::int64> (blockStart, blockEnd));

    // 4. Hand new marks to the granulator — it finds finished grains to reuse.
    if (!synthMarks.empty())
        mGranulator.generateGrains (synthMarks);

    // 5. Overlap-add all active grains into the output buffer.
    //    The write range of each grain is offset by lookaheadSamples, so we
    //    query the granulator at the lookahead-adjusted block window.
    const juce::int64 lookahead = mGranulator.getLookaheadSize();
    buffer.clear();
    mGranulator.process (buffer, blockStart + lookahead, blockEnd + lookahead);

    mAbsoluteSampleCount += numSamples;
}

//==============================================================================
bool TDPSOLA_Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Input and output layouts must match; mono or stereo only.
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
