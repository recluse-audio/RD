/**
 * @file GranulatorProcessor.h
 * @author Ryan Devens
 * @brief AudioProcessor wrapper for Granulator utility
 * @version 0.1
 * @date 2025-02-01
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include "Util/Juce_Header.h"
#include "Granulator.h"
#include "../../CircularBuffer.h"
#include "../../PITCH/PitchDetector.h"

/**
 * @brief Processor that wraps Granulator for TD-PSOLA granular synthesis.
 *
 * This processor provides a juce::AudioProcessor interface for the Granulator
 * utility class, allowing it to be used as a node in an AudioProcessorGraph.
 *
 * Implements TD-PSOLA (Time-Domain Pitch Synchronous Overlap-Add) algorithm:
 * - Detects pitch using YIN algorithm
 * - Tracks stable pitch marks using peak detection and prediction
 * - Performs pitch-shifting by time-stretching with grain overlap-add
 */
class GranulatorProcessor : public juce::AudioProcessor
                          , public juce::AudioProcessorValueTreeState::Listener
{
public:
    GranulatorProcessor();
    ~GranulatorProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override { juce::ignoreUnused(layouts); return true; }

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override          { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override                        { return true;   }

    const juce::String getName() const override            { return "Granulator Processor"; }
    bool acceptsMidi() const override                      { return false; }
    bool producesMidi() const override                     { return false; }
    double getTailLengthSeconds() const override           { return 0; }

    //==============================================================================
    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const juce::String getProgramName (int) override             { return "None"; }
    void changeProgramName (int, const juce::String&) override   {}

    //
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        juce::ignoreUnused(destData);
    }

    //
    void setStateInformation (const void* data, int sizeInBytes) override
    {
        juce::ignoreUnused(data, sizeInBytes);
    }

    juce::AudioProcessorValueTreeState& getAPVTS();

   // AudioProcessorValueTreeState::Listener callback
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Access to internal components for testing
    Granulator& getGranulator() { return mGranulator; }
    CircularBuffer& getCircularBuffer() { return mCircularBuffer; }
    PitchDetector* getPitchDetector() { return mPitchDetector.get(); }

    // Processing state
    enum class ProcessState
    {
        kDetecting = 0, // Looking for pitch (initial detection, noise before)
        kTracking = 1   // Tracking pitch (after at least 1 detection)
    };
    ProcessState getCurrentState() const { return mProcessState; }

    // Range calculation methods (exposed for testing)
    std::tuple<juce::int64, juce::int64> getProcessCounterRange();
    std::tuple<juce::int64, juce::int64> getDetectionRange();
    std::tuple<juce::int64, juce::int64> getFirstPeakRange(float detectedPeriod);
    std::tuple<juce::int64, juce::int64> getPrecisePeakRange(juce::int64 predictedAnalysisMark, float detectedPeriod);
    std::tuple<juce::int64, juce::int64, juce::int64> getAnalysisReadRange(juce::int64 analysisMark, float detectedPeriod);
    std::tuple<juce::int64, juce::int64, juce::int64> getAnalysisWriteRange(std::tuple<juce::int64, juce::int64, juce::int64> analysisReadRange);
    std::tuple<juce::int64, juce::int64> getDryBlockRange();

private:
    int mBlockSize = 512;
    double mSampleRate = 44100;

    // Core components
    std::unique_ptr<PitchDetector> mPitchDetector;
    Granulator mGranulator;
    CircularBuffer mCircularBuffer;
    juce::AudioBuffer<float> mDetectionBuffer;

    // State tracking
    ProcessState mProcessState = ProcessState::kDetecting;
    juce::int64 mSamplesProcessed = 0;
    juce::int64 mPredictedNextAnalysisMark = -1;

    // Parameters
    juce::Atomic<float> mShiftRatio{1.0f}; // 0.5 to 1.5 (default 1.0 = no shift)

    // Constants
    static constexpr int kMinLookaheadSize = 512;
    static constexpr int kMinDetectionSize = 1024;
    static constexpr int kMaxGrainSize = 4096;
    static constexpr int kCircularBufferSize = 8192;

    // Core processing methods
    float doDetection(juce::AudioBuffer<float>& processBuffer);
    void doCorrection(juce::AudioBuffer<float>& processBuffer, float detectedPeriod);
    juce::int64 chooseStablePitchMark(juce::int64 endDetectionSample, float detectedPeriod);

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout _createParameterLayout();

    // cleanup ugly code in PluginProcessor's constructor
    juce::AudioProcessor::BusesProperties _getBusesProperties();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranulatorProcessor)
};
