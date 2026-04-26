/**
 * test_YIN_PitchDetector.cpp
 * Created by Ryan Devens
 *
 * Tests for YIN_PitchDetector class functionality (YIN/CMND algorithm)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "TEST_UTILS/TestUtils.h"
#include "../SOURCE/PITCH/YIN_PitchDetector.h"
#include "../SOURCE/BUFFER_FILLER/BufferFiller.h"

//==============================================================================
class YIN_PitchDetectorTester
{
public:
	static int getHalfBlock(const YIN_PitchDetector& pd) { return pd.mHalfBlock; }
	static int getDifferenceBufferNumSamples(const YIN_PitchDetector& pd) { return pd.differenceBuffer.getNumSamples(); }
	static int getDifferenceBufferNumChannels(const YIN_PitchDetector& pd) { return pd.differenceBuffer.getNumChannels(); }
	static int getCmndBufferNumSamples(const YIN_PitchDetector& pd) { return pd.cmndBuffer.getNumSamples(); }
	static int getCmndBufferNumChannels(const YIN_PitchDetector& pd) { return pd.cmndBuffer.getNumChannels(); }
};

//==============================================================================
namespace TestConfig
{
	constexpr double sampleRate = 48000.0;
	constexpr int detectionSize = YIN_PitchDetectorConstants::DefaultDetectionSize;  // 4096
	constexpr int expectedHalfBlock = detectionSize / 2; // 2048
}

//==============================================================================

TEST_CASE("YIN_PitchDetector prepareToPlay() initializes member objects correctly", "[YIN_PitchDetector][prepareToPlay]")
{
	TestUtils::SetupAndTeardown setup;
	YIN_PitchDetector detector;

	SECTION("Before prepareToPlay: default values from constructor")
	{
		CHECK(YIN_PitchDetectorTester::getHalfBlock(detector) == 0);

		CHECK(YIN_PitchDetectorTester::getDifferenceBufferNumSamples(detector) == YIN_PitchDetectorConstants::DefaultDetectionSize / 2);
		CHECK(YIN_PitchDetectorTester::getDifferenceBufferNumChannels(detector) == 1);
		CHECK(YIN_PitchDetectorTester::getCmndBufferNumSamples(detector) == YIN_PitchDetectorConstants::DefaultDetectionSize / 2);
		CHECK(YIN_PitchDetectorTester::getCmndBufferNumChannels(detector) == 1);
	}

	detector.prepareToPlay(TestConfig::detectionSize);

	SECTION("After prepareToPlay: mHalfBlock is blockSize / 2")
	{
		CHECK(YIN_PitchDetectorTester::getHalfBlock(detector) == TestConfig::expectedHalfBlock);
	}

	SECTION("After prepareToPlay: differenceBuffer is resized correctly")
	{
		CHECK(YIN_PitchDetectorTester::getDifferenceBufferNumChannels(detector) == 1);
		CHECK(YIN_PitchDetectorTester::getDifferenceBufferNumSamples(detector) == TestConfig::expectedHalfBlock);
	}

	SECTION("After prepareToPlay: cmndBuffer is resized correctly")
	{
		CHECK(YIN_PitchDetectorTester::getCmndBufferNumChannels(detector) == 1);
		CHECK(YIN_PitchDetectorTester::getCmndBufferNumSamples(detector) == TestConfig::expectedHalfBlock);
	}
}

TEST_CASE("YIN_PitchDetector prepareToPlay() with different detection sizes", "[YIN_PitchDetector][prepareToPlay]")
{
	TestUtils::SetupAndTeardown setup;
	YIN_PitchDetector detector;

	SECTION("2048 sample detection buffer")
	{
		detector.prepareToPlay(2048);
		CHECK(YIN_PitchDetectorTester::getHalfBlock(detector) == 1024);
	}

	SECTION("4096 sample detection buffer")
	{
		detector.prepareToPlay(4096);
		CHECK(YIN_PitchDetectorTester::getHalfBlock(detector) == 2048);
	}
}

//==============================================================================

TEST_CASE("YIN_PitchDetector process() detects sine wave period", "[YIN_PitchDetector][process]")
{
	TestUtils::SetupAndTeardown setup;
	constexpr int detectionSize = YIN_PitchDetectorConstants::DefaultDetectionSize;
	constexpr int sinePeriod = 256;
	constexpr int numChannels = 1;

	YIN_PitchDetector detector;
	detector.prepareToPlay(detectionSize);
	detector.setThreshold(0.01);

	juce::AudioBuffer<float> sineBuffer(numChannels, detectionSize); sineBuffer.clear();
	BufferFiller::generateSineCycles(sineBuffer, (double)sinePeriod, 0.0);

	SECTION("Detects period of 256 samples from sine wave")
	{
		float detectedPeriod = detector.process(sineBuffer);

		// YIN algorithm should detect period close to 256 samples
		CHECK(detectedPeriod == Catch::Approx(static_cast<float>(sinePeriod)).margin(1.0f));
	}
}

TEST_CASE("YIN_PitchDetector process() with various frequencies", "[YIN_PitchDetector][process]")
{
	TestUtils::SetupAndTeardown setup;
	constexpr int detectionSize = YIN_PitchDetectorConstants::DefaultDetectionSize;
	constexpr int numChannels = 1;

	YIN_PitchDetector detector;
	detector.prepareToPlay(detectionSize);
	detector.setThreshold(0.01);

	SECTION("100 Hz sine wave (480 samples period at 48kHz)")
	{
		constexpr int period = 480;
		juce::AudioBuffer<float> buffer(numChannels, detectionSize);
		buffer.clear();
		BufferFiller::generateSineCycles(buffer, period);

		float detectedPeriod = detector.process(buffer);
		CHECK(detectedPeriod == Catch::Approx(static_cast<float>(period)).margin(5.0f));
	}

	SECTION("200 Hz sine wave (240 samples period at 48kHz)")
	{
		constexpr int period = 240;
		juce::AudioBuffer<float> buffer(numChannels, detectionSize);
		buffer.clear();
		BufferFiller::generateSineCycles(buffer, period);

		float detectedPeriod = detector.process(buffer);
		CHECK(detectedPeriod == Catch::Approx(static_cast<float>(period)).margin(2.0f));
	}

	SECTION("440 Hz sine wave (109 samples period at 48kHz)")
	{
		constexpr int period = 109;
		juce::AudioBuffer<float> buffer(numChannels, detectionSize);
		buffer.clear();
		BufferFiller::generateSineCycles(buffer, period);

		float detectedPeriod = detector.process(buffer);
		CHECK(detectedPeriod == Catch::Approx(static_cast<float>(period)).margin(2.0f));
	}
}

TEST_CASE("YIN_PitchDetector returns -1 for silent or noise input", "[YIN_PitchDetector][process]")
{
	TestUtils::SetupAndTeardown setup;
	constexpr int detectionSize = YIN_PitchDetectorConstants::DefaultDetectionSize;
	constexpr int numChannels = 1;

	YIN_PitchDetector detector;
	detector.prepareToPlay(detectionSize);

	SECTION("Silent buffer returns -1")
	{
		juce::AudioBuffer<float> buffer(numChannels, detectionSize);
		buffer.clear();

		float detectedPeriod = detector.process(buffer);
		CHECK(detectedPeriod == -1.0f);
	}
}

TEST_CASE("YIN_PitchDetector getCurrentPeriod() thread-safe getter", "[YIN_PitchDetector][thread-safety]")
{
	TestUtils::SetupAndTeardown setup;
	constexpr int detectionSize = YIN_PitchDetectorConstants::DefaultDetectionSize;
	constexpr int sinePeriod = 256;
	constexpr int numChannels = 1;

	YIN_PitchDetector detector;
	detector.prepareToPlay(detectionSize);

	juce::AudioBuffer<float> sineBuffer(numChannels, detectionSize);
	sineBuffer.clear();
	BufferFiller::generateSineCycles(sineBuffer, sinePeriod);

	float detectedPeriod = detector.process(sineBuffer);

	double storedPeriod = detector.getCurrentPeriod();
	CHECK(storedPeriod == Catch::Approx(static_cast<double>(detectedPeriod)).margin(0.1));
}

TEST_CASE("YIN_PitchDetector direct usage", "[YIN_PitchDetector]")
{
	TestUtils::SetupAndTeardown setup;
	SECTION("Direct YIN_PitchDetector usage")
	{
		constexpr int detectionSize = YIN_PitchDetectorConstants::DefaultDetectionSize;
		const float expectedPeriod = 200;

		YIN_PitchDetector detector;
		detector.prepareToPlay(detectionSize);
		detector.setThreshold(0.01f);

		juce::AudioBuffer<float> sineBuffer(1, detectionSize);
		BufferFiller::generateSineCycles(sineBuffer, expectedPeriod, 0.0);

		float detectedPeriod = detector.process(sineBuffer);

		INFO("Expected period: " << expectedPeriod);
		INFO("Detected period: " << detectedPeriod);

		CHECK(detectedPeriod > 0);
		CHECK(detectedPeriod == Catch::Approx(expectedPeriod).margin(5.0f));
	}
}
