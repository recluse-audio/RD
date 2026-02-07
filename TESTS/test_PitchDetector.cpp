/**
 * test_PitchDetector.cpp
 * Created by Ryan Devens
 *
 * Tests for PitchDetector class functionality
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "../SOURCE/PITCH/PitchDetector.h"
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/PROCESSORS/GRAIN/GranulatorProcessor.h"

//==============================================================================
// Test Access Class - Provides access to private members for testing
//==============================================================================
class PitchDetectorTester
{
public:
	static int getHalfBlock(const PitchDetector& pd) { return pd.mHalfBlock; }
	static int getDifferenceBufferNumSamples(const PitchDetector& pd) { return pd.differenceBuffer.getNumSamples(); }
	static int getDifferenceBufferNumChannels(const PitchDetector& pd) { return pd.differenceBuffer.getNumChannels(); }
	static int getCmndBufferNumSamples(const PitchDetector& pd) { return pd.cmndBuffer.getNumSamples(); }
	static int getCmndBufferNumChannels(const PitchDetector& pd) { return pd.cmndBuffer.getNumChannels(); }
};

//==============================================================================
// Test Constants
//==============================================================================
namespace TestConfig
{
	constexpr double sampleRate = 48000.0;
	constexpr int detectionSize = PitchDetectorMagicNumbers::DefaultDetectionSize;  // 4096
	constexpr int expectedHalfBlock = detectionSize / 2; // 2048
}

//==============================================================================
// prepareToPlay() Tests
//==============================================================================

TEST_CASE("PitchDetector prepareToPlay() initializes member objects correctly", "[PitchDetector][prepareToPlay]")
{
	PitchDetector detector;

	SECTION("Before prepareToPlay: default values from constructor")
	{
		// Default mHalfBlock is 0 (set in class definition)
		CHECK(PitchDetectorTester::getHalfBlock(detector) == 0);

		// Default buffer size from constructor is DefaultDetectionSize / 2
		CHECK(PitchDetectorTester::getDifferenceBufferNumSamples(detector) == PitchDetectorMagicNumbers::DefaultDetectionSize / 2);
		CHECK(PitchDetectorTester::getDifferenceBufferNumChannels(detector) == 1);
		CHECK(PitchDetectorTester::getCmndBufferNumSamples(detector) == PitchDetectorMagicNumbers::DefaultDetectionSize / 2);
		CHECK(PitchDetectorTester::getCmndBufferNumChannels(detector) == 1);
	}

	// Call prepareToPlay with test configuration
	detector.prepareToPlay(TestConfig::detectionSize);

	SECTION("After prepareToPlay: mHalfBlock is blockSize / 2")
	{
		CHECK(PitchDetectorTester::getHalfBlock(detector) == TestConfig::expectedHalfBlock);
	}

	SECTION("After prepareToPlay: differenceBuffer is resized correctly")
	{
		CHECK(PitchDetectorTester::getDifferenceBufferNumChannels(detector) == 1);
		CHECK(PitchDetectorTester::getDifferenceBufferNumSamples(detector) == TestConfig::expectedHalfBlock);
	}

	SECTION("After prepareToPlay: cmndBuffer is resized correctly")
	{
		CHECK(PitchDetectorTester::getCmndBufferNumChannels(detector) == 1);
		CHECK(PitchDetectorTester::getCmndBufferNumSamples(detector) == TestConfig::expectedHalfBlock);
	}
}

TEST_CASE("PitchDetector prepareToPlay() with different detection sizes", "[PitchDetector][prepareToPlay]")
{
	PitchDetector detector;

	SECTION("2048 sample detection buffer")
	{
		detector.prepareToPlay(2048);
		CHECK(PitchDetectorTester::getHalfBlock(detector) == 1024);
	}

	SECTION("4096 sample detection buffer")
	{
		detector.prepareToPlay(4096);
		CHECK(PitchDetectorTester::getHalfBlock(detector) == 2048);
	}
}

//==============================================================================
// process() Tests
//==============================================================================

TEST_CASE("PitchDetector process() detects sine wave period", "[PitchDetector][process]")
{
	constexpr int detectionSize = PitchDetectorMagicNumbers::DefaultDetectionSize;
	constexpr int sinePeriod = 256;
	constexpr int numChannels = 1;

	// Create and prepare detector
	PitchDetector detector;
	detector.prepareToPlay(detectionSize);
	detector.setThreshold(0.01);

	// Create buffer and fill with sine wave cycles (period = 256 samples)
	juce::AudioBuffer<float> sineBuffer(numChannels, detectionSize); sineBuffer.clear();
	BufferFiller::generateSineCycles(sineBuffer, (double)sinePeriod, 0.0);

	SECTION("Detects period of 256 samples from sine wave")
	{
		float detectedPeriod = detector.process(sineBuffer);

		// YIN algorithm should detect period close to 256 samples
		CHECK(detectedPeriod == Catch::Approx(static_cast<float>(sinePeriod)).margin(1.0f));
	}
}

TEST_CASE("PitchDetector process() with various frequencies", "[PitchDetector][process]")
{
	constexpr int detectionSize = PitchDetectorMagicNumbers::DefaultDetectionSize;
	constexpr int numChannels = 1;

	PitchDetector detector;
	detector.prepareToPlay(detectionSize);
	detector.setThreshold(0.01);

	SECTION("100 Hz sine wave (480 samples period at 48kHz)")
	{
		// 48000 / 100 = 480 samples per period
		constexpr int period = 480;
		juce::AudioBuffer<float> buffer(numChannels, detectionSize);
		buffer.clear();
		BufferFiller::generateSineCycles(buffer, period);

		float detectedPeriod = detector.process(buffer);
		CHECK(detectedPeriod == Catch::Approx(static_cast<float>(period)).margin(5.0f));
	}

	SECTION("200 Hz sine wave (240 samples period at 48kHz)")
	{
		// 48000 / 200 = 240 samples per period
		constexpr int period = 240;
		juce::AudioBuffer<float> buffer(numChannels, detectionSize);
		buffer.clear();
		BufferFiller::generateSineCycles(buffer, period);

		float detectedPeriod = detector.process(buffer);
		CHECK(detectedPeriod == Catch::Approx(static_cast<float>(period)).margin(2.0f));
	}

	SECTION("440 Hz sine wave (109 samples period at 48kHz)")
	{
		// 48000 / 440 ≈ 109 samples per period
		constexpr int period = 109;
		juce::AudioBuffer<float> buffer(numChannels, detectionSize);
		buffer.clear();
		BufferFiller::generateSineCycles(buffer, period);

		float detectedPeriod = detector.process(buffer);
		CHECK(detectedPeriod == Catch::Approx(static_cast<float>(period)).margin(2.0f));
	}
}

TEST_CASE("PitchDetector returns -1 for silent or noise input", "[PitchDetector][process]")
{
	constexpr int detectionSize = PitchDetectorMagicNumbers::DefaultDetectionSize;
	constexpr int numChannels = 1;

	PitchDetector detector;
	detector.prepareToPlay(detectionSize);

	SECTION("Silent buffer returns -1")
	{
		juce::AudioBuffer<float> buffer(numChannels, detectionSize);
		buffer.clear();

		float detectedPeriod = detector.process(buffer);
		CHECK(detectedPeriod == -1.0f);
	}
}

TEST_CASE("PitchDetector getCurrentPeriod() thread-safe getter", "[PitchDetector][thread-safety]")
{
	constexpr int detectionSize = PitchDetectorMagicNumbers::DefaultDetectionSize;
	constexpr int sinePeriod = 256;
	constexpr int numChannels = 1;

	PitchDetector detector;
	detector.prepareToPlay(detectionSize);

	juce::AudioBuffer<float> sineBuffer(numChannels, detectionSize);
	sineBuffer.clear();
	BufferFiller::generateSineCycles(sineBuffer, sinePeriod);

	// Process to detect period
	float detectedPeriod = detector.process(sineBuffer);

	// Verify atomic getter returns same value
	double storedPeriod = detector.getCurrentPeriod();
	CHECK(storedPeriod == Catch::Approx(static_cast<double>(detectedPeriod)).margin(0.1));
}

TEST_CASE("PitchDetector with GranulatorProcessor", "[PitchDetector][GranulatorProcessor]")
{
	SECTION("Direct PitchDetector usage (not through GranulatorProcessor)")
	{
		constexpr int detectionSize = PitchDetectorMagicNumbers::DefaultDetectionSize;
		const float expectedPeriod = 200;

		// Create PitchDetector directly and prepare it with the detectionSize
		PitchDetector detector;
		detector.prepareToPlay(detectionSize);
		detector.setThreshold(0.01f)
;
		// Create buffer matching the detectionSize (YIN needs at least 2*halfBlock samples)
		juce::AudioBuffer<float> sineBuffer(1, detectionSize);
		BufferFiller::generateSineCycles(sineBuffer, expectedPeriod, 0.0);

		float detectedPeriod = detector.process(sineBuffer);

		INFO("Expected period: " << expectedPeriod);
		INFO("Detected period: " << detectedPeriod);

		CHECK(detectedPeriod > 0);
		CHECK(detectedPeriod == Catch::Approx(expectedPeriod).margin(5.0f));
	}

	SECTION("GranulatorProcessor pitch detection (accounts for internal buffer sizing)")
	{
		constexpr double sampleRate = 48000.0;
		constexpr int blockSize = 512;
		const float expectedPeriod = 200;

		GranulatorProcessor processor;
		processor.prepareToPlay(sampleRate, blockSize);
		processor.getPitchDetector()->setThreshold(0.01f)
;		// GranulatorProcessor uses a specific detection buffer size
		// Get the actual size from the processor's pitch detector setup
		const int pitchDetectBufferSize = PitchDetectorMagicNumbers::DefaultDetectionSize;

		// Create buffer matching what PitchDetector expects
		juce::AudioBuffer<float> sineBuffer(1, pitchDetectBufferSize);
		BufferFiller::generateSineCycles(sineBuffer, expectedPeriod, 0.0);

		float detectedPeriod = processor.getPitchDetector()->process(sineBuffer);

		INFO("Expected period: " << expectedPeriod);
		INFO("Detected period: " << detectedPeriod);

		CHECK(detectedPeriod > 0);
		CHECK(detectedPeriod == Catch::Approx(expectedPeriod).margin(5.0f));
	}
}
