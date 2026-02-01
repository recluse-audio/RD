/**
 * GranulatorProcessor.cpp
 * Created by Ryan Devens
 */

#include "GranulatorProcessor.h"

GranulatorProcessor::GranulatorProcessor()
: AudioProcessor (_getBusesProperties())
, apvts(*this, nullptr, "Parameters", _createParameterLayout())
{
    // Add parameter listeners
    apvts.addParameterListener("shiftRatio", this);
}

GranulatorProcessor::~GranulatorProcessor()
{
    apvts.removeParameterListener("shiftRatio", this);
}

void GranulatorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    mBlockSize = samplesPerBlock;
    mSamplesProcessed = 0;

    // Scale detection buffer for higher sample rates
    int detectBufferSize = std::max(samplesPerBlock * 2, kMinDetectionSize);
    if (sampleRate > 48000 && sampleRate <= 96000)
        detectBufferSize *= 2;
    else if (sampleRate > 96000)
        detectBufferSize *= 4;

    // Initialize pitch detector
    mPitchDetector = std::make_unique<PitchDetector>();
    mPitchDetector->prepareToPlay(sampleRate, detectBufferSize);

    // Prepare circular buffer for delay/lookahead
    mCircularBuffer.setSize(2, kCircularBufferSize);

    // Prepare granulator
    mGranulator.prepare(sampleRate, samplesPerBlock, kMaxGrainSize);

    // Prepare detection buffer
    mDetectionBuffer.setSize(1, detectBufferSize);
    mDetectionBuffer.clear();

    // Reset state
    mProcessState = ProcessState::kDetecting;
    mPredictedNextAnalysisMark = -1;
}

void GranulatorProcessor::releaseResources()
{
    mPitchDetector.reset();
}

void GranulatorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Write input to circular buffer
    for (int i = 0; i < numSamples; ++i)
    {
        int writeIndex = mCircularBuffer.getWrappedIndex(mSamplesProcessed + i);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float sample = buffer.getSample(ch, i);
            mCircularBuffer.getBuffer().setSample(ch, writeIndex, sample);
        }
    }

    // Clear output buffer before processing
    buffer.clear();
    mDetectionBuffer.clear();

    // Detect pitch
    float detectedPeriod = doDetection(buffer);

    // Process based on detection result
    if (detectedPeriod > 0)
    {
        // Pitch detected - perform correction/shifting
        doCorrection(buffer, detectedPeriod);
        mProcessState = ProcessState::kTracking;
    }
    else
    {
        // No pitch detected - pass through or process in detecting mode
        auto dryRange = getDryBlockRange();
        auto processRange = getProcessCounterRange();

        mGranulator.processDetecting(buffer, mCircularBuffer, dryRange, processRange);

        // Reset tracking state on detection loss
        if (mProcessState == ProcessState::kTracking)
        {
            mPredictedNextAnalysisMark = -1;
            mGranulator.resetSynthMark();
            mProcessState = ProcessState::kDetecting;
        }
    }

    mSamplesProcessed += numSamples;
}

//===================
//
void GranulatorProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "shiftRatio")
    {
        float clampedValue = juce::jlimit(0.5f, 1.5f, newValue);
        mShiftRatio.set(clampedValue);
    }
}

//===================
// Core Processing Methods
//===================

float GranulatorProcessor::doDetection(juce::AudioBuffer<float>& processBuffer)
{
    // Get detection range (delayed by kMinLookaheadSize)
    auto [detectStart, detectEnd] = getDetectionRange();

    // Read from circular buffer into detection buffer
    for (int i = 0; i < mDetectionBuffer.getNumSamples(); ++i)
    {
        juce::int64 readPos = detectStart + i;
        int wrappedIndex = mCircularBuffer.getWrappedIndex(readPos);
        // Use mono (channel 0) for pitch detection
        float sample = mCircularBuffer.getBuffer().getSample(0, wrappedIndex);
        mDetectionBuffer.setSample(0, i, sample);
    }

    // Try to detect pitch
    float detectedPeriod = mPitchDetector->process(mDetectionBuffer);
    return detectedPeriod;
}

//===================
//
juce::int64 GranulatorProcessor::chooseStablePitchMark(juce::int64 endDetectionSample, float detectedPeriod)
{
    juce::int64 startDetectionSample = endDetectionSample - kMinDetectionSize;

    // If we have a prediction and it's inside the detection window, use narrow search
    if (mPredictedNextAnalysisMark >= startDetectionSample &&
        mPredictedNextAnalysisMark <= endDetectionSample)
    {
        juce::int64 radius = static_cast<juce::int64>(std::llround(detectedPeriod * 0.25f));

        juce::int64 rs = mPredictedNextAnalysisMark - radius;
        juce::int64 re = mPredictedNextAnalysisMark + radius;

        // Clamp to detection window
        rs = juce::jmax(rs, startDetectionSample);
        re = juce::jmin(re, endDetectionSample);

        juce::Range<juce::int64> searchRange(rs, re);
        return mCircularBuffer.findPeakInRange(searchRange, 0);
    }

    // Otherwise, fall back to wide search (one period at end of detection window)
    {
        juce::int64 re = endDetectionSample;
        juce::int64 rs = re - static_cast<juce::int64>(std::llround(detectedPeriod));

        juce::Range<juce::int64> searchRange(rs, re);
        return mCircularBuffer.findPeakInRange(searchRange, 0);
    }
}

//===================
//
void GranulatorProcessor::doCorrection(juce::AudioBuffer<float>& processBuffer, float detectedPeriod)
{
    // Calculate shifted period based on shift ratio
    float shiftedPeriod = detectedPeriod / mShiftRatio.get();

    juce::int64 endProcessSample = mSamplesProcessed + mBlockSize - 1;
    juce::int64 endDetectionSample = endProcessSample - kMinLookaheadSize;

    // Find stable analysis mark
    juce::int64 analysisMark = chooseStablePitchMark(endDetectionSample, detectedPeriod);

    // Predict next analysis mark for temporal stability
    mPredictedNextAnalysisMark = analysisMark + static_cast<juce::int64>(std::llround(detectedPeriod));

    // Calculate ranges
    auto analysisReadRange = getAnalysisReadRange(analysisMark, detectedPeriod);
    auto analysisWriteRange = getAnalysisWriteRange(analysisReadRange);
    auto processCounterRange = getProcessCounterRange();

    // Process with granulator
    mGranulator.processTracking(
        processBuffer,
        mCircularBuffer,
        analysisReadRange,
        analysisWriteRange,
        processCounterRange,
        detectedPeriod,
        shiftedPeriod
    );
}


//===================
// Range Calculation Methods
//===================

std::tuple<juce::int64, juce::int64> GranulatorProcessor::getProcessCounterRange()
{
    juce::int64 startProcessSample = mSamplesProcessed;
    juce::int64 endProcessSample = startProcessSample + mBlockSize - 1;
    return std::make_tuple(startProcessSample, endProcessSample);
}

//===================
//
std::tuple<juce::int64, juce::int64> GranulatorProcessor::getDetectionRange()
{
    juce::int64 endProcessSample = mSamplesProcessed + mBlockSize - 1;
    juce::int64 endDetectionSample = endProcessSample - kMinLookaheadSize;
    juce::int64 startDetectionSample = endDetectionSample - kMinDetectionSize;
    return std::make_tuple(startDetectionSample, endDetectionSample);
}

//===================
//
std::tuple<juce::int64, juce::int64> GranulatorProcessor::getFirstPeakRange(float detectedPeriod)
{
    juce::int64 endProcessSample = mSamplesProcessed + mBlockSize - 1;
    juce::int64 endDetectionSample = endProcessSample - kMinLookaheadSize;
    juce::int64 endFirstPeakRange = endDetectionSample;
    juce::int64 startFirstPeakRange = endFirstPeakRange - static_cast<juce::int64>(detectedPeriod);
    return std::make_tuple(startFirstPeakRange, endFirstPeakRange);
}

//===================
//
std::tuple<juce::int64, juce::int64> GranulatorProcessor::getPrecisePeakRange(juce::int64 predictedAnalysisMark, float detectedPeriod)
{
    juce::int64 radius = static_cast<juce::int64>(detectedPeriod * 0.25f);
    juce::int64 predictedRangeStart = predictedAnalysisMark - radius;
    juce::int64 predictedRangeEnd = predictedAnalysisMark + radius;
    return std::make_tuple(predictedRangeStart, predictedRangeEnd);
}

//===================
//
std::tuple<juce::int64, juce::int64, juce::int64> GranulatorProcessor::getAnalysisReadRange(juce::int64 analysisMark, float detectedPeriod)
{
    juce::int64 analysisRangeStart = analysisMark - static_cast<juce::int64>(detectedPeriod);
    juce::int64 analysisRangeEnd = analysisMark + static_cast<juce::int64>(detectedPeriod) - 1;
    return std::make_tuple(analysisRangeStart, analysisMark, analysisRangeEnd);
}

//===================
//
std::tuple<juce::int64, juce::int64, juce::int64> GranulatorProcessor::getAnalysisWriteRange(std::tuple<juce::int64, juce::int64, juce::int64> analysisReadRange)
{
    juce::int64 writeStart = std::get<0>(analysisReadRange) + kMinLookaheadSize;
    juce::int64 writeMark = std::get<1>(analysisReadRange) + kMinLookaheadSize;
    juce::int64 writeEnd = std::get<2>(analysisReadRange) + kMinLookaheadSize;
    return std::make_tuple(writeStart, writeMark, writeEnd);
}

//===================
//
std::tuple<juce::int64, juce::int64> GranulatorProcessor::getDryBlockRange()
{
    juce::int64 blockRangeStart = mSamplesProcessed - kMinLookaheadSize;
    juce::int64 blockRangeEnd = blockRangeStart + mBlockSize;
    return std::make_tuple(blockRangeStart, blockRangeEnd);
}

//==================================
// PRIVATE
//==================================

//===================
//
juce::AudioProcessorValueTreeState::ParameterLayout GranulatorProcessor::_createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Shift ratio parameter (0.5 = octave down, 1.0 = no shift, 1.5 = fifth up)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "shiftRatio",       // Parameter ID
        "Shift Ratio",      // Parameter name
        juce::NormalisableRange<float>(0.5f, 1.5f, 0.01f),
        1.0f));            // Default value (no shift)

    return { params.begin(), params.end() };
}

//====================
//
juce::AudioProcessor::BusesProperties GranulatorProcessor::_getBusesProperties()
{
    return BusesProperties()
                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                .withOutput("Output", juce::AudioChannelSet::stereo(), true);
}

//====================
//
juce::AudioProcessorValueTreeState& GranulatorProcessor::getAPVTS()
{
    return apvts;
}
