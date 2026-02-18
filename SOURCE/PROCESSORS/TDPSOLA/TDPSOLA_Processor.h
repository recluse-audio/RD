/**
 * TDPSOLA_Processor.h
 * Created by Ryan Devens
 *
 * juce::AudioProcessor that performs TD-PSOLA pitch shifting in real-time.
 *
 * Owns and coordinates three components:
 *   - CircularBuffer   : stores incoming audio for pitch-mark–based reading
 *   - PitchManager     : accumulates audio, runs pitch detection, tracks pitch
 *                        and synth marks in absolute sample time
 *   - TD_Granulator    : maintains a pool of TD_Grain objects; each grain reads
 *                        from the CircularBuffer and overlap-adds into the output
 *
 * Processing flow per block:
 *   1. CircularBuffer::pushBuffer() stores the incoming audio
 *   2. PitchManager::process() detects pitch and tracks synth marks in absolute time
 *   3. SynthMarks covering the current output window are fetched from PitchManager
 *   4. TD_Granulator::generateGrains() assigns marks to available grains
 *   5. TD_Granulator::process() overlap-adds all active grains into the output buffer
 *
 * Declaration order of members is intentional:
 *   mCircularBuffer must be fully constructed before mGranulator, which holds a
 *   reference to it.
 */

#pragma once
#include "Util/Juce_Header.h"
#include "CircularBuffer.h"
#include "PITCH/PitchManager.h"
#include "TD_Granulator.h"

class TDPSOLA_Processor : public juce::AudioProcessor
{
public:
    TDPSOLA_Processor();
    ~TDPSOLA_Processor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    //==============================================================================
    // AudioProcessor boilerplate
    juce::AudioProcessorEditor* createEditor() override         { return nullptr; }
    bool hasEditor() const override                             { return false; }
    const juce::String getName() const override                 { return "TDPSOLA Processor"; }
    bool acceptsMidi() const override                           { return false; }
    bool producesMidi() const override                          { return false; }
    double getTailLengthSeconds() const override                { return 0.0; }
    int getNumPrograms() override                               { return 1; }
    int getCurrentProgram() override                            { return 0; }
    void setCurrentProgram (int) override                       {}
    const juce::String getProgramName (int) override            { return "None"; }
    void changeProgramName (int, const juce::String&) override  {}
    void getStateInformation (juce::MemoryBlock& d) override    { juce::ignoreUnused(d); }
    void setStateInformation (const void* d, int n) override    { juce::ignoreUnused(d, n); }

    //==============================================================================
    // Accessors for testing and editor access
    CircularBuffer&  getCircularBuffer()  { return mCircularBuffer; }
    PitchManager&    getPitchManager()    { return mPitchManager; }
    TD_Granulator&   getGranulator()      { return mGranulator; }

    juce::int64 getAbsoluteSampleCount() const { return mAbsoluteSampleCount; }

private:
    // mCircularBuffer MUST be declared before mGranulator —
    // TD_Granulator holds a reference to it and is initialised via member init list.
    CircularBuffer mCircularBuffer;
    PitchManager   mPitchManager;
    TD_Granulator  mGranulator;

    double       mSampleRate          = 44100.0;
    int          mBlockSize           = 512;
    juce::int64  mAbsoluteSampleCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TDPSOLA_Processor)
};
