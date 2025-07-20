#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+

#include "TEST_UTILS/TestUtils.h"

#include "../SOURCE/AudioFileProcessor.h"
#include "../SOURCE/RelativeFilePath.h"
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/BufferHelper.h"
#include "../SOURCE/BufferRange.h"
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


//==========
TEST_CASE("Get wrapped index of buffer.")
{
    juce::AudioBuffer<float> buffer(2, 20);

    CHECK(BufferHelper::getWrappedIndex(buffer, 0) == 0);
    CHECK(BufferHelper::getWrappedIndex(buffer, 20) == 0);

    CHECK(BufferHelper::getWrappedIndex(buffer, 21) == 1);
    CHECK(BufferHelper::getWrappedIndex(buffer, -1) == 19);

}

//===========
TEST_CASE("Get Interpolated Sample")
{
	juce::AudioBuffer<float> buffer(1, 10);
	BufferFiller::fillIncremental(buffer);

	float fractionalIndex_0to1 = 0.83f;
	float interpolatedSample_0to1 = BufferHelper::getLinearInterpolatedSampleAtIndex(buffer, fractionalIndex_0to1);
	CHECK(interpolatedSample_0to1 == Catch::Approx(fractionalIndex_0to1).margin(0.0001f));

	float fractionalIndex_1to2 = 1.0f + juce::Random::getSystemRandom().nextFloat(); // range [1.0, 2.0)
	float interpolatedSample_1to2 = BufferHelper::getLinearInterpolatedSampleAtIndex(buffer, fractionalIndex_1to2);

	// Optional: If you want to test it against a known value (e.g., buffer is linearly increasing)
	CHECK(interpolatedSample_1to2 == Catch::Approx(fractionalIndex_1to2).margin(0.0001f));

}

//================
TEST_CASE("Can get range of buffer as AudioBlock")
{
	juce::AudioBuffer<float> incrementalBuffer(2, 100);
	BufferFiller::fillIncremental(incrementalBuffer);
	RD::BufferRange readRange(10, 20);
	juce::dsp::AudioBlock<float> readBlock = BufferHelper::getRangeAsBlock(incrementalBuffer, readRange);

	CHECK(readBlock.getNumChannels() == incrementalBuffer.getNumChannels());
	CHECK(readBlock.getNumSamples() == readRange.getLengthInSamples());

	for(juce::int64 readIndex = 0; readIndex < readRange.getLengthInSamples(); readIndex++)
	{
		for(int ch = 0; ch < incrementalBuffer.getNumChannels(); ch++)
		{
			juce::int64 bufferReadIndex = readIndex + readRange.getStartIndex();
			float bufferSample = incrementalBuffer.getSample(ch, (int)bufferReadIndex);
			float blockSample = readBlock.getSample(ch, readIndex);

			CHECK(bufferSample == blockSample);

		}
	}
}

//================
TEST_CASE("Can write juce::dsp::AudioBlock to juce::AudioBuffer at specific juce::Range")
{
	juce::AudioBuffer<float> incBuffer(2, 10);
	BufferFiller::fillIncremental(incBuffer);
	juce::dsp::AudioBlock<float> incBlock(incBuffer);

	// Technically testing the juce code here, or atleast that it works as we think it does
	CHECK(incBlock.getNumChannels() == incBuffer.getNumChannels());
	CHECK(incBlock.getNumSamples() == incBuffer.getNumSamples());

	// our incBuffer is smaller than this one, so we can tell if it is written to the correct range
	juce::AudioBuffer<float> outputBuffer(2, 30);
	outputBuffer.clear();

	// writing the incBlock values to the outputBuffer in this range
	RD::BufferRange writeRange(10, 19);
	CHECK(writeRange.getLengthInSamples() == 10);
	// do the writing
	BufferHelper::writeBlockToBuffer(outputBuffer, incBlock, writeRange);

	// output indices 0-9, we can expect 0's
	for(juce::int64 sampleIndex = 0; sampleIndex < writeRange.getStartIndex(); sampleIndex++)
	{		
		for(int ch = 0; ch < outputBuffer.getNumChannels(); ch++)
		{
			float outputSample = outputBuffer.getSample(ch, sampleIndex);
			CHECK(outputSample == 0.f);
		}
	}

	// now in the range of the "writing" so we should see the incBlock vals appear
	for(juce::int64 sampleIndex = writeRange.getStartIndex(); sampleIndex <= writeRange.getEndIndex(); sampleIndex++)
	{
		int indexInBlock = (int)(sampleIndex - writeRange.getStartIndex());

		for(int ch = 0; ch < outputBuffer.getNumChannels(); ch++)
		{
			float outputSample = outputBuffer.getSample(ch, sampleIndex);
			float blockSample = incBlock.getSample(ch, indexInBlock);
			CHECK(outputSample == blockSample);
		}
	}

	// back out of the writing portion
	for(juce::int64 sampleIndex = writeRange.getEndIndex() + 1; sampleIndex < (juce::int64)outputBuffer.getNumSamples(); sampleIndex++)
	{
		for(int ch = 0; ch < outputBuffer.getNumChannels(); ch++)
		{
			float outputSample = outputBuffer.getSample(ch, (int)sampleIndex);
			CHECK(outputSample == 0.f);
		}
	}
}