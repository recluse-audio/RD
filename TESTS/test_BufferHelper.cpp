#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+

#include "TEST_UTILS/TestUtils.h"
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

//================
TEST_CASE("Apply window to audio block")
{
	TestUtils::SetupAndTeardown setupAndTeardown;

	const int blockSize = 128;

	// Create a buffer filled with all ones
	juce::AudioBuffer<float> buffer(1, blockSize);
	BufferFiller::fillWithAllOnes(buffer);

	// Create an audio block from the buffer
	juce::dsp::AudioBlock<float> block(buffer);

	// Create and configure window with Hanning shape
	Window window;
	window.setSizeShapePeriod(1024, Window::Shape::kHanning, blockSize);

	// Apply window to block
	BufferHelper::applyWindowToBlock(block, window);

	// known final value when interpolating 1024->128 (not zero)
	const float knownFinalValue = 0.00046f;

	// Verify that the block values are now windowed
	// At index 0 and blockSize-1, Hanning should be ~0.0
	CHECK(block.getSample(0, 0) == Catch::Approx(0.0f).margin(0.0001f));
	CHECK(block.getSample(0, blockSize - 1) == Catch::Approx(knownFinalValue).margin(0.00001f));

	// At the middle (index 64), Hanning should be ~1.0
	CHECK(block.getSample(0, blockSize / 2) == Catch::Approx(1.0f).epsilon(0.01f));

	// Verify all values are between 0 and 1
	for (int i = 0; i < blockSize; ++i)
	{
		float sample = block.getSample(0, i);
		CHECK(sample >= 0.0f);
		CHECK(sample <= 1.0f);
	}
}

//================
TEST_CASE("Window with period 128 reads every 8th value from size 1024 buffer")
{
	TestUtils::SetupAndTeardown setupAndTeardown;

	const int windowSize = 1024;
	const int period = 128;
	const int phaseIncrement = windowSize / period;  // Should be 8

	// Load the full 1024-sample Hanning window from golden file
	juce::AudioBuffer<float> goldenFullHanning;
	goldenFullHanning.clear();

	juce::File currentDir = juce::File::getCurrentWorkingDirectory();
	juce::String relativePath = "/SUBMODULES/RD/TESTS/GOLDEN/GOLDEN_HanningBuffer_1024.csv";
	juce::String fullPath = currentDir.getFullPathName() + relativePath;

	bool loadSuccess = BufferFiller::loadFromCSV(goldenFullHanning, fullPath);
	REQUIRE(loadSuccess == true);
	REQUIRE(goldenFullHanning.getNumSamples() == windowSize);

	INFO("Loaded " << goldenFullHanning.getNumSamples() << " samples from GOLDEN_HanningBuffer_1024.csv");
	INFO("Window size: " << windowSize << ", Period: " << period << ", Phase increment: " << phaseIncrement);

	// Create Window with size 1024 and period 128
	Window window;
	window.setSizeShapePeriod(windowSize, Window::Shape::kHanning, period);

	// Read through the window for one period (128 samples)
	// Each getNextSample() should advance by phase increment of 8
	// So we should get: goldenFullHanning[0], goldenFullHanning[8], goldenFullHanning[16], ...
	for (int i = 0; i < period; ++i)
	{
		float windowValue = window.getNextSample();
		int expectedIndex = i * phaseIncrement;
		float expectedValue = goldenFullHanning.getSample(0, expectedIndex);

		INFO("Sample " << i << ": window value = " << windowValue
			 << ", expected (golden[" << expectedIndex << "]) = " << expectedValue);
		CHECK(windowValue == Catch::Approx(expectedValue).epsilon(0.0001f));
	}
}