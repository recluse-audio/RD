#pragma once

#include "Util/Juce_Header.h"
#include "PitchMarkBuffer.h"
#include "PitchMarkHistory.h"

class CircularBuffer;
class PitchDetector;
class Granulator;
class Window;

#if (MSVC)
#include "ipps.h"
#endif

namespace MagicNumbers
{
	constexpr int minLookaheadSize = 1024; // for synthesis
    constexpr int minDetectionSize = 2048; // for detection
} // end namespace MagicNumbers
class GranulatorProcessor : public juce::AudioProcessor
                      , public juce::AudioProcessorValueTreeState::Listener
{
public:
    GranulatorProcessor();
    ~GranulatorProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    float doDetection(juce::AudioBuffer<float>& processBuffer);
    void doCorrection(juce::AudioBuffer<float>& processBuffer, float detectedPeriod);
    void processDry(juce::AudioBuffer<float>& processBuffer);
    juce::int64 refineMarkByCorrelation(juce::int64 predictedMark, float detectedPeriod);
    inline float readMonoSample(juce::int64 sampleIndex) const;
    juce::int64 chooseStablePitchMark(const juce::int64 endDetectionSample, const float detectedPeriod);

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
	
	//===================================
    const float getLastDetectedPitch();
    const float getLastDetectedPeriod();
    const float getShiftRatio() const { return mShiftRatio; }

    juce::AudioProcessorValueTreeState& getAPVTS();
    Granulator& getGranulator() { return *mGranulator.get(); }
    PitchDetector* getPitchDetector() { return mPitchDetector.get(); }
    CircularBuffer& getCircularBuffer() { return *mCircularBuffer.get(); }

    // Pitch mark accessors (realtime-safe - return const references)
    const PitchMarkBuffer& getCurrentBlockMarks() const { return mCurrentBlockMarks; }
    const PitchMarkHistory& getMarkHistory() const { return mAnalysisMarkHistory; }

   // AudioProcessorValueTreeState::Listener callback
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // range of current process block relative to total num processed, no delay
    std::tuple<juce::int64, juce::int64> getProcessCounterRange();

    // range of current process block in delayed coordinate system (matches grain positions)
    std::tuple<juce::int64, juce::int64> getDelayedProcessCounterRange();

    // starts at delayed position behind process counter range
    std::tuple<juce::int64, juce::int64> getDetectionRange();

    // when we've detected a pitch, this is the range of a complete cycle nearest the end of the detection buffer
    std::tuple<juce::int64, juce::int64> getFirstPeakRange(float detectedPeriod);

    std::tuple<juce::int64, juce::int64> getPrecisePeakRange(juce::int64 predictedAnalysisMark, float detectedPeriod);

    std::tuple<juce::int64, juce::int64, juce::int64> getAnalysisReadRange(juce::int64 analysisMark, float detectedPeriod);

    // same as analysisReadRange but offset by minLookaheadSize (undelayed write position)
    std::tuple<juce::int64, juce::int64, juce::int64> getAnalysisWriteRange(std::tuple<juce::int64, juce::int64, juce::int64> analysisReadRange);

    // happens when no pitch is detected and we want to let dry signal back through, but still delayed
    std::tuple<juce::int64, juce::int64> getDryBlockRange();


private:
    float mShiftRatio = 1.f;
    std::unique_ptr<PitchDetector> mPitchDetector;
    std::unique_ptr<Granulator> mGranulator;
    std::unique_ptr<CircularBuffer> mCircularBuffer;

	juce::AudioBuffer<float> mDetectionBuffer;

	juce::int64 mSamplesProcessed = 0;
	int mBlockSize = 0;
    juce::int64 mPredictedNextAnalysisMark = (juce::int64) -1;

    // Current analysis mark and ranges (reused until exhausted)
    juce::int64 mCurrentAnalysisMark = -1;
    std::tuple<juce::int64, juce::int64, juce::int64> mCurrentAnalysisReadRange{-1, -1, -1};
    std::tuple<juce::int64, juce::int64, juce::int64> mCurrentAnalysisWriteRange{-1, -1, -1};
    bool mHasValidAnalysisMark = false;

    // Pitch mark storage
    PitchMarkBuffer mCurrentBlockMarks;      // Marks found in current block only
    PitchMarkHistory mAnalysisMarkHistory;   // Long-term mark history (ring buffer)


    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout _createParameterLayout();

    void _initParameterListeners();
    // cleanup ugly code in GranulatorProcessor's constructor
    juce::AudioProcessor::BusesProperties _getBusesProperties();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranulatorProcessor)
};
