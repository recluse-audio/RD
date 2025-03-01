#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+

#include "TEST_UTILS/TestUtils.h"

#include "../SOURCE/AudioFileProcessor.h"
#include "../SOURCE/RelativeFilePath.h"
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/BufferHelper.h"
#include "../SOURCE/AudioFileHelpers.h"
#include "../SOURCE/EFFECTS/GainProcessor.h"

TEST_CASE("True for identical buffers")
{
    int numChannels = 1;
    int numSamples = 10;

    juce::AudioBuffer<float> buffer1(numChannels, numSamples);
    juce::AudioBuffer<float> buffer2(numChannels, numSamples);

    buffer1.clear();
    buffer2.clear();

    BufferFiller::fillWithAllOnes(buffer1);
    BufferFiller::fillWithAllOnes(buffer2);
    
    CHECK(BufferHelper::buffersAreIdentical(buffer1, buffer2) == true);

}

// 
TEST_CASE("Test different sample values")
{
    int numChannels = 1;
    int numSamples = 10;

    juce::AudioBuffer<float> buffer1(numChannels, numSamples);
    juce::AudioBuffer<float> buffer2(numChannels, numSamples);

    buffer1.clear();
    buffer2.clear();

    // different fill methods, different sample values
    BufferFiller::fillWithAllOnes(buffer1);
    BufferFiller::fillIncremental(buffer2); 
    
    CHECK(BufferHelper::buffersAreIdentical(buffer1, buffer2) == false);

}

// 
TEST_CASE("Test different numChannels")
{
    int numChannels = 1;
    int numSamples = 10;

    // give this to buffer2
    int altNumChannels = 2;

    juce::AudioBuffer<float> buffer1(numChannels, numSamples);
    juce::AudioBuffer<float> buffer2(altNumChannels, numSamples);

    buffer1.clear();
    buffer2.clear();

    // different fill methods, different sample values
    BufferFiller::fillWithAllOnes(buffer1);
    BufferFiller::fillWithAllOnes(buffer2); 
    
    CHECK(BufferHelper::buffersAreIdentical(buffer1, buffer2) == false);

}

// 
TEST_CASE("Test different numSamples")
{
    int numChannels = 1;
    int numSamples = 10;

    // give this to buffer2
    int altNumSamples = 11;

    juce::AudioBuffer<float> buffer1(numChannels, numSamples);
    juce::AudioBuffer<float> buffer2(numChannels, altNumSamples);

    buffer1.clear();
    buffer2.clear();

    // different fill methods, different sample values
    BufferFiller::fillWithAllOnes(buffer1);
    BufferFiller::fillWithAllOnes(buffer2); 
    
    CHECK(BufferHelper::buffersAreIdentical(buffer1, buffer2) == false);

}


TEST_CASE("Get Max RMS")
{
    juce::AudioBuffer<float> buffer(2, 20);

    BufferFiller::fillChannelWithValue(buffer, 0, 1.f);
    BufferFiller::fillChannelWithValue(buffer, 1, 2.f);

    float maxRMS = BufferHelper::getMaxRMS(buffer);
    CHECK(maxRMS == 2.f);

}


TEST_CASE("Detect silent buffer")
{
    juce::AudioBuffer<float> buffer(2, 20);

    BufferFiller::fillWithValue(buffer, 1.f);
    bool isSilent = BufferHelper::isSilent(buffer);
    CHECK(isSilent == false);

    BufferFiller::fillWithValue(buffer, 0.f);
    isSilent = BufferHelper::isSilent(buffer);
    CHECK(isSilent == true);

    BufferFiller::fillWithValue(buffer, -1.f);
    isSilent = BufferHelper::isSilent(buffer);
    CHECK(isSilent == false);

}

TEST_CASE("Process buffer with gain processor")
{
    TestUtils::SetupAndTeardown setupAndTeardown;

    {
        
        int numSamples = 4096;
        int blockSize = 512; 

        juce::AudioBuffer<float> buffer(1, numSamples);
        BufferFiller::fillWithAllOnes(buffer);

        GainProcessor gainProcessor;
        gainProcessor.prepareToPlay(44100, blockSize);
        gainProcessor.setGain(0.1f);

        BufferHelper::processBuffer(buffer, gainProcessor, blockSize);

        for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
        {
            auto sample = buffer.getSample(0, sampleIndex);
            CHECK(sample == 0.1f);
        }

    }
    juce::DeletedAtShutdown::deleteAll();

}