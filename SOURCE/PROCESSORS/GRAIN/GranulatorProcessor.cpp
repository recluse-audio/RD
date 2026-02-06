#include "GranulatorProcessor.h"
#include "Granulator.h"
#include "../../PITCH/PitchDetector.h"
#include "../../CircularBuffer.h"
#include "../../BufferHelper.h"


//==============================================================================
GranulatorProcessor::GranulatorProcessor()
: AudioProcessor (_getBusesProperties())
, apvts(*this, nullptr, "Parameters", _createParameterLayout())
{
    mPitchDetector = std::make_unique<PitchDetector>();
    mCircularBuffer = std::make_unique<CircularBuffer>();
	mGranulator = std::make_unique<Granulator>();

	mShiftRatio = 1.f;

    _initParameterListeners();

}

//=================================
//
GranulatorProcessor::~GranulatorProcessor()
{
	mCircularBuffer.reset();
    mPitchDetector.reset();
    mGranulator.reset();
}

//==============================================================================
const juce::String GranulatorProcessor::getName() const
{
    return "Granulator Processor";
}

bool GranulatorProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GranulatorProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GranulatorProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GranulatorProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GranulatorProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int GranulatorProcessor::getCurrentProgram()
{
    return 0;
}

void GranulatorProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String GranulatorProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void GranulatorProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void GranulatorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	// be atleast minLookaheadSize, if at or above, use 2x block size
	int pitchDetectBufferNumSamples = samplesPerBlock >= MagicNumbers::minDetectionSize ? (samplesPerBlock * 2) : MagicNumbers::minDetectionSize;
    int latencySamples = MagicNumbers::minLookaheadSize;

	// scale for sample rates, we deal with the same size for 44100 and 48000 for now (same for 88200 and 96000)
	if(sampleRate > 48000.0 && sampleRate <= 96000.0)
    {
		pitchDetectBufferNumSamples = pitchDetectBufferNumSamples * 2;
        latencySamples = latencySamples * 2;
    }
	else if(sampleRate > 96000.0)
    {
		pitchDetectBufferNumSamples = pitchDetectBufferNumSamples * 4;
        latencySamples = latencySamples * 4;
    }

	mDetectionBuffer.clear();
	mDetectionBuffer.setSize(getTotalNumOutputChannels(), pitchDetectBufferNumSamples);

    mPitchDetector->prepareToPlay(pitchDetectBufferNumSamples);

    mCircularBuffer->setSize(getTotalNumOutputChannels(), static_cast<int>(pitchDetectBufferNumSamples) * 2); // by default 2 seconds
    //mCircularBuffer->setDelay(MagicNumbers::minLookaheadSize);  // delay is factored in as part of getAnalysisReadRange

    // Maximum grain size: Detected periods can be up to pitchDetectBufferNumSamples/2 (mHalfBlock),
    // and grains are 2*period, so maxGrainSize needs to be pitchDetectBufferNumSamples (not pitchDetectBufferNumSamples/2)
    int maxGrainSize = pitchDetectBufferNumSamples;
    mGranulator->prepare(sampleRate, samplesPerBlock, maxGrainSize);

	mSamplesProcessed = 0;
	mBlockSize = samplesPerBlock;
    mPredictedNextAnalysisMark = -1;

    // Initialize pitch mark storage
    mCurrentBlockMarks.clear();
    mAnalysisMarkHistory.prepareToPlay(sampleRate, samplesPerBlock);

    setLatencySamples(latencySamples);
}

void GranulatorProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool GranulatorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    juce::ignoreUnused(layouts); 
    return true;
//   #if JucePlugin_IsMidiEffect
//     juce::ignoreUnused (layouts);
//     return true;
//   #else
//     // This is the place where you check if the layout is supported.
//     // In this template code we only support mono or stereo.
//     if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
//      && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
//         return false;

//     // This checks if the input layout matches the output layout
//    #if ! JucePlugin_IsSynth
//     if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
//         return false;
//    #endif

//     return true;
//   #endif
}

void GranulatorProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    [[maybe_unused]] auto totalNumInputChannels  = getTotalNumInputChannels();
    [[maybe_unused]] auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear pitch marks from previous block (realtime-safe, just resets counter)
    mCurrentBlockMarks.clear();

    // write audio to circular buffer
    bool writeSuccess = mCircularBuffer->pushBuffer(buffer);
	if(!writeSuccess)
		return;

    // invalid values, bail out

    // clean up buffers, about to fill
	buffer.clear();
    mDetectionBuffer.clear();

    // Wait until enough samples processed to fill detection buffer with valid data
    // Detection buffer size is typically 1024 samples. Need at least 2x that for clean detection.
    const juce::int64 minSamplesForDetection = mDetectionBuffer.getNumSamples() * 2;
    float detected_period = -1.0f;

    if (mSamplesProcessed >= minSamplesForDetection)
    {
        detected_period = doDetection(buffer);
    }

    static int blockCounter = 0;
    if (blockCounter < 25)
    {
        std::cout << "Block #" << blockCounter << " - detected_period: " << detected_period
                  << ", mSamplesProcessed: " << mSamplesProcessed
                  << ", minSamplesForDetection: " << minSamplesForDetection << std::endl;
        blockCounter++;
    }

    // Always fill buffer with delayed dry audio first
    // This ensures uncovered samples have the correct delayed input (not zeros)
    // Grains will overlap-add on top of this where they have coverage
    processDry(buffer);

    // If pitch detected, apply granular correction on top of dry audio
    if (detected_period > 0)
    {
        doCorrection(buffer, detected_period);
    }

	mSamplesProcessed += buffer.getNumSamples();
}

//=============================================================================
float GranulatorProcessor::doDetection(juce::AudioBuffer<float>& processBuffer)
{
    // range we will detect on
    auto [detectStart, detectEnd] = getDetectionRange();
    mCircularBuffer->readRange(mDetectionBuffer, detectStart);

    // Amplify quiet audio for better pitch detection
    // YIN algorithm needs sufficient signal level to work reliably
    float rms = mDetectionBuffer.getRMSLevel(0, 0, mDetectionBuffer.getNumSamples());
    const float targetRMS = 0.2f;  // Target RMS for pitch detection
    const float minRMS = 0.001f;   // Avoid amplifying pure silence

    if (rms > minRMS && rms < targetRMS)
    {
        float gain = targetRMS / rms;
        // Limit gain to avoid extreme amplification of noise
        gain = juce::jmin(gain, 50.0f);
        mDetectionBuffer.applyGain(gain);

        static int gainLogCount = 0;
        if (gainLogCount < 3)
        {
            std::cout << "Amplifying detection buffer: RMS " << rms
                      << " -> " << mDetectionBuffer.getRMSLevel(0, 0, mDetectionBuffer.getNumSamples())
                      << " (gain: " << gain << "x)" << std::endl;
            gainLogCount++;
        }
    }

    // Try and detect pitch, update state accordingly in temp variable for now
    float detected_period = mPitchDetector->process(mDetectionBuffer);
    return detected_period;
}

//=============================================================================
void GranulatorProcessor::doCorrection(juce::AudioBuffer<float>& processBuffer, float detectedPeriod)
{
    static bool loggedOnce = false;
    if (!loggedOnce)
    {
        std::cout << "PITCH SHIFT DEBUG - mShiftRatio: " << mShiftRatio
                  << ", detectedPeriod: " << detectedPeriod
                  << ", shiftedPeriod: " << (detectedPeriod / mShiftRatio) << std::endl;
        loggedOnce = true;
    }

    const float shiftedPeriod = detectedPeriod / mShiftRatio;

    const juce::int64 endProcessSample   = mSamplesProcessed + mBlockSize - 1;
    const juce::int64 endDetectionSample = endProcessSample - MagicNumbers::minLookaheadSize;

    // Only find a new analysis mark when needed
    // Check if current mark has been exhausted (synthMark advanced beyond its range)
    bool needNewMark = !mHasValidAnalysisMark;

    if (mHasValidAnalysisMark)
    {
        // Check if synthMark has advanced past the current analysis mark's output range
        juce::int64 currentSynthMark = mGranulator->getSynthMark();
        juce::int64 currentAnalysisWriteEnd = std::get<2>(mCurrentAnalysisWriteRange);

        // Need new mark if synthMark has reached or passed the end of current analysis range
        if (currentSynthMark >= currentAnalysisWriteEnd)
        {
            needNewMark = true;
        }
    }

    if (needNewMark)
    {
        // Find new analysis mark
        const juce::int64 markedIndex = chooseStablePitchMark(endDetectionSample, detectedPeriod);

        // Store the found mark in current block buffer (realtime-safe)
        mCurrentBlockMarks.addMark(markedIndex);

        // Add to long-term history (realtime-safe, writes to pre-allocated ring buffer)
        mAnalysisMarkHistory.addMark(markedIndex);

        // Prediction for NEXT time (in the SAME coordinate system as markedIndex)
        mPredictedNextAnalysisMark = markedIndex + (juce::int64)std::llround(detectedPeriod);

        // Calculate and store ranges for this mark
        mCurrentAnalysisMark = markedIndex;
        mCurrentAnalysisReadRange  = getAnalysisReadRange(markedIndex, detectedPeriod);
        mCurrentAnalysisWriteRange = getAnalysisWriteRange(mCurrentAnalysisReadRange);
        mHasValidAnalysisMark = true;
    }

    // Fill processBuffer with delayed dry audio first (fallback for samples without grain coverage)
    auto [dryStart, dryEnd] = getDryBlockRange();
    mCircularBuffer->readRange(processBuffer, dryStart);

    // Use current mark to create grains (will replace samples where grains contribute)
    mGranulator->processTracking(
        processBuffer,
        *mCircularBuffer.get(),
        mCurrentAnalysisReadRange,
        mCurrentAnalysisWriteRange,
        getProcessCounterRange(),  // processBuffer represents delayed output timeline
        detectedPeriod,
        shiftedPeriod);
}

//=============================================================================
void GranulatorProcessor::processDry(juce::AudioBuffer<float>& processBuffer)
{
    // Read dry signal from circular buffer with lookahead delay
    auto [dryStart, dryEnd] = getDryBlockRange();
    mCircularBuffer->readRange(processBuffer, dryStart);
}

//==================================================================
juce::int64 GranulatorProcessor::refineMarkByCorrelation(juce::int64 predictedMark, float detectedPeriod)
{
    const int P = (int)std::llround(detectedPeriod);
    const int radius = std::max(1, P / 4);

    // Reference cycle: one period ending at predictedMark (you can use prevMark instead if you store it)
    std::vector<float> ref(P);
    for (int i = 0; i < P; ++i)
        ref[i] = readMonoSample(predictedMark - P + i);

    double bestScore = -1.0;
    juce::int64 bestMark = predictedMark;

    for (int off = -radius; off <= radius; ++off)
    {
        const juce::int64 cand = predictedMark + off;

        double num = 0.0, denA = 0.0, denB = 0.0;
        for (int i = 0; i < P; ++i)
        {
            const float a = ref[i];
            const float b = readMonoSample(cand - P + i); // compare same-relative cycle
            num  += (double)a * (double)b;
            denA += (double)a * (double)a;
            denB += (double)b * (double)b;
        }

        const double score = num / (std::sqrt(denA * denB) + 1e-12);
        if (score > bestScore)
        {
            bestScore = score;
            bestMark = cand;
        }
    }

    return bestMark;
}

//==================================================================
juce::int64 GranulatorProcessor::chooseStablePitchMark( const juce::int64 endDetectionSample, const float detectedPeriod)
{
    const juce::int64 startDetectionSample = endDetectionSample - MagicNumbers::minDetectionSize;
    const juce::int64 periodInt = (juce::int64)std::llround(detectedPeriod);

    juce::int64 searchStart, searchEnd;
    bool usedPrediction = false;

    // Check if prediction is valid
    if (mPredictedNextAnalysisMark > 0)
    {
        // Always use the prediction without adjustment to maintain smooth grain spacing
        // Even if prediction is slightly outside detection window, the search will be clamped
        const juce::int64 radius = periodInt / 4;
        searchStart = mPredictedNextAnalysisMark - radius;
        searchEnd = mPredictedNextAnalysisMark + radius;
        usedPrediction = true;
    }
    else
    {
        // No prediction yet - search the last period of the detection window
        searchStart = endDetectionSample - periodInt;
        searchEnd = endDetectionSample;
    }

    // Don't clamp search range - circular buffer has valid data even outside detection window
    // The prediction-based search ensures we stay near the correct phase
    juce::Range<juce::int64> r(searchStart, searchEnd);
    juce::int64 foundMark = mCircularBuffer->findPeakInRange(r, 0);

    // Use correlation refinement if we have enough processing history
    // Need at least 2 periods of history before foundMark for correlation to work
    const juce::int64 minSamplesForCorrelation = periodInt * 2;
    if (mSamplesProcessed >= minSamplesForCorrelation && foundMark >= minSamplesForCorrelation)
    {
        // Refine using correlation for phase continuity
        foundMark = refineMarkByCorrelation(foundMark, detectedPeriod);
    }

    return foundMark;
}


//==============================================================================
inline float GranulatorProcessor::readMonoSample(juce::int64 sampleIndex) const
{
    // If your CircularBuffer has a direct getter, use it.
    // Otherwise adapt to however you read a single sample.
    const int ch = 0;
    const int wrapped = mCircularBuffer->getWrappedIndex(sampleIndex);
    return mCircularBuffer->getBuffer().getSample(ch, wrapped);
}


//==============================================================================
bool GranulatorProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GranulatorProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void GranulatorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void GranulatorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
//===============================================================================
//
const float GranulatorProcessor::getLastDetectedPitch()
{
    float pitch = static_cast<float>(this->getSampleRate() / mPitchDetector->getCurrentPeriod());
    return pitch;
}

//===============================================================================
//
const float GranulatorProcessor::getLastDetectedPeriod()
{
    return static_cast<float>(mPitchDetector->getCurrentPeriod());
}

//===============================================================================
//
juce::AudioProcessorValueTreeState& GranulatorProcessor::getAPVTS()
{
    return apvts;
}

//===============================================================================
//
void GranulatorProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if(parameterID == "shift ratio")
    {
        float clampedValue = juce::jlimit(0.5f, 1.5f, newValue);
		mShiftRatio = clampedValue;
    }
    else if(parameterID == "emission rate")
    {
    }
}

//==================================
// PRIVATE
//==================================

//===================
//
juce::AudioProcessorValueTreeState::ParameterLayout GranulatorProcessor::_createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Add a gain parameter as an example
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "shift ratio",         // Parameter ID
        "Shift Ratio",         // Parameter name
        0.5,           // Min value
        1.5f,           // Max value
        1.f));         // Default value

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "emission rate",         // Parameter ID
        "Emission Rate",         // Parameter name
        1.f,           // Min value
        400.f,           // Max value
        1.f));         // Default value
        
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

//===================
void GranulatorProcessor::_initParameterListeners()
{
    apvts.addParameterListener("shift ratio", this);
    apvts.addParameterListener("emission rate", this);
}

//-------------------------------------------
std::tuple<juce::int64, juce::int64> GranulatorProcessor::getProcessCounterRange()
{
    // where we will be at the end of this block
    juce::int64 startProcessSample = mSamplesProcessed;
    juce::int64 endProcessSample = startProcessSample + mBlockSize - 1;

    return std::make_tuple(startProcessSample, endProcessSample);
}

//-------------------------------------------
std::tuple<juce::int64, juce::int64> GranulatorProcessor::getDelayedProcessCounterRange()
{
    // Process range in delayed coordinate system (matches grain positions)
    // This ensures grains overlap-add correctly with the output buffer
    juce::int64 delayedStart = mSamplesProcessed - MagicNumbers::minLookaheadSize;
    juce::int64 delayedEnd = delayedStart + mBlockSize - 1;

    return std::make_tuple(delayedStart, delayedEnd);
}

//-------------------------------------------
std::tuple<juce::int64, juce::int64> GranulatorProcessor::getDetectionRange()
{
    // where we will be at the end of this block
    juce::int64 endProcessSample = mSamplesProcessed + mBlockSize - 1; 
    // The end sample index of windowed audio data, but adjusted by lookahead
    juce::int64 endDetectionSample = endProcessSample - MagicNumbers::minLookaheadSize;
    juce::int64 startDetectionSample = endDetectionSample - MagicNumbers::minDetectionSize;
    return std::make_tuple(startDetectionSample, endDetectionSample);
}


//-------------------------------------------
std::tuple<juce::int64, juce::int64> GranulatorProcessor::getFirstPeakRange(float detectedPeriod)
{
    // where we will be at the end of this block
    juce::int64 endProcessSample = mSamplesProcessed + mBlockSize - 1; 
    juce::int64 endDetectionSample = endProcessSample - MagicNumbers::minLookaheadSize;
    // The end sample index of windowed audio data, but adjusted by lookahead
    juce::int64 endFirstPeakRange = endDetectionSample; // 
    juce::int64 startFirstPeakRange = endFirstPeakRange - (juce::int64)detectedPeriod;
    return std::make_tuple(startFirstPeakRange, endFirstPeakRange);
}

//-------------------------------------------
std::tuple<juce::int64, juce::int64> GranulatorProcessor::getPrecisePeakRange(juce::int64 predictedAnalysisMarker, float detectionPeriod)
{
    // where we will be at the end of this block
    juce::int64 radius = (juce::int64)(detectionPeriod * 0.25f);
    juce::int64 predictedRangeStart = predictedAnalysisMarker - radius;
    juce::int64 predictedRangeEnd = predictedAnalysisMarker + radius;
    return std::make_tuple(predictedRangeStart, predictedRangeEnd);
}

//-------------------------------------------
std::tuple<juce::int64, juce::int64, juce::int64> GranulatorProcessor::getAnalysisReadRange(juce::int64 analysisMark, float detectedPeriod)
{
    juce::int64 analysisRangeStart = analysisMark - (juce::int64) detectedPeriod;
    juce::int64 analysisRangeEnd = analysisMark + (juce::int64) detectedPeriod - 1;
    return std::make_tuple(analysisRangeStart, analysisMark, analysisRangeEnd);
}

//-------------------------------------------
std::tuple<juce::int64, juce::int64, juce::int64> GranulatorProcessor::getAnalysisWriteRange(std::tuple<juce::int64, juce::int64, juce::int64> analysisReadRange)
{
    // Synthesis positions compensate for lookahead delay
    // Analysis reads from delayed audio, synthesis writes to current timeline
    juce::int64 writeStart = std::get<0>(analysisReadRange) + MagicNumbers::minLookaheadSize;
    juce::int64 writeMark = std::get<1>(analysisReadRange) + MagicNumbers::minLookaheadSize;
    juce::int64 writeEnd = std::get<2>(analysisReadRange) + MagicNumbers::minLookaheadSize;
    return std::make_tuple(writeStart, writeMark, writeEnd);
}

//-------------------------------------------
std::tuple<juce::int64, juce::int64> GranulatorProcessor::getDryBlockRange()
{
    juce::int64 blockRangeStart = mSamplesProcessed - MagicNumbers::minLookaheadSize;
    juce::int64 blockRangeEnd = blockRangeStart + mBlockSize;
    return std::make_tuple(blockRangeStart, blockRangeEnd);
}


