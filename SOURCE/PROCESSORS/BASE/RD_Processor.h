/**
 * RD_Processor.h
 * Created by Ryan Devens on 2026-04-23 (with love ofc)
 *
 * Base class from which all my processors shall inherit until I regret doing so.
 *
 * Owns `mBaseAPVTS` (a default APVTS carrying a "gain" parameter) so that
 * RD_Processor is not abstract and can be instantiated on its own.
 *
 * Derived classes declare their own `mAPVTS` and override getAPVTS() to
 * return a reference to it.
 */

#pragma once
#include "Util/Juce_Header.h"
#include "../../DATA_LOGGER/DataLogger.h"

class RD_Processor : public juce::AudioProcessor
                   , public juce::AudioProcessorValueTreeState::Listener
                   , public DataLogger
{
public:
    enum class LifecycleState
    {
        kIdle,
        kPreparedToPlay,
        kProcessBlockStart,
        kProcessBlockEnd
    };

    RD_Processor();
    ~RD_Processor() override;

    LifecycleState getLifecycleState() const { return mLifecycleState; }

    //==========================================================================
    //================== PROCESS BLOCK FINAL OVERRIDE ==========================
    //==========================================================================
    // Child classes should override `doProcessBlock()`, see below
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) final override;
    //===========================================================================
    //=============================================================================

    //==============================================================================
    //=============== VIRTUAL FUNCTIONS / PARENT CLASS OVERRIDES ===================
    //==============================================================================

    //--------------------------------
    // RD_PROCESSOR virtual functions
    //--------------------------------

    /**
     * Called by RD_Processor::processBlock(), override this instead of base processBlock()
     * So RD_Processor base class can keep some data logging logic and other stuff
     * out of your child class. If you don't want to do this, then you may not want to use this processor.
     */

    virtual void doProcessBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiBuffer);

    /**
     * Called by RD_Processor::prepareToPlay(), override this instead of base prepareToPlay()
     * so RD_Processor base class can fire the kPreparedToPlay lifecycle log.
     */
    virtual void doPrepareToPlay (double sampleRate, int samplesPerBlock);
   
    /** 
      Returns the APVTS for this processor. Derived classes override to
        return a reference to their own `mAPVTS`. 
    */
    virtual juce::AudioProcessorValueTreeState& getAPVTS();

    //------------------------------------------------
    // Should probably override these in child classes
    //------------------------------------------------
    const juce::String getName() const override { return "RD_Processor"; } 
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    //==============================================================================
   
    //=============================================================================
    //============ OVERRIDE THESE IN CHILD CLASSES =====================

    void prepareToPlay (double sampleRate, int samplesPerBlock) final override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    const double getLastSampleRateFromPrepareToPlay() const;
    const int    getLastBlockSizeFromPrepareToPlay()  const;

    juce::int64 getProcessSampleCount() const;

    void startLogging();
    void stopLogging();

    bool doLogData() override;

    // data logging specific to RD_Processor
    juce::File createProcessorDataLogFile();

    void setGain (float newGain);

protected:
    double      mSampleRate         = 44100.0;
    int         mBlockSize           = 512;
    juce::int64 mProcessSampleCount  = 0;

    juce::Atomic<float>                mGainValue;
    juce::AudioProcessorValueTreeState mBaseAPVTS;

    LifecycleState mLifecycleState { LifecycleState::kIdle };

    juce::AudioBuffer<float> mLogBuffer;
    juce::int64              mLogBlockStartIndex = 0;

private:
    static BusesProperties _getDefaultBusesProperties();
    static juce::AudioProcessorValueTreeState::ParameterLayout _createParameterLayout();
    void _updateGainValue (float newValue);

    void _fireLifecycleLog (LifecycleState state);

    bool _logPrepareToPlay();
    bool _logProcessBlockStart();
    bool _logProcessBlockEnd();

    void _writeBlockSamplesCsv (const juce::String& filenamePrefix);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RD_Processor)
};
