#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+

#include "../SOURCE/BufferHelper.h"
#include "../SOURCE/BufferFiller.h"

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