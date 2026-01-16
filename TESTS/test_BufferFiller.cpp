// TODO, move this to a common or DSP_TOOLS test folder

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/BufferHelper.h"
#include "../SOURCE/RelativeFilePath.h"


TEST_CASE("Can make a Hanning Window", "[BufferFiller]")
{
    juce::AudioBuffer<float> buffer(1, 1024);
    int numSamples = buffer.getNumSamples() - 1; // doing this extra step to make it look like the implementation

    BufferFiller::generateHanning(buffer);

    CHECK(buffer.getSample(0, 0) == Catch::Approx(0.0f).epsilon(0.0001f));
    CHECK(buffer.getSample(0, numSamples) == Catch::Approx(0.0f).epsilon(0.0001f));

    // checking values are normalized
    for(int sampleIndex = 0; sampleIndex <= numSamples; sampleIndex++)
    {
        float sample = buffer.getSample(0, sampleIndex);
        CHECK(sample >= 0.f);
        CHECK(sample <= 1.f);
    }

    // checking symmetry
    for(int sampleIndex = 0; sampleIndex <= numSamples / 2.f; sampleIndex++)
    {
        float sample1 = buffer.getSample(0, sampleIndex);
        float sample2 = buffer.getSample(0, numSamples - sampleIndex);

        CHECK(sample1 == Catch::Approx(sample2).margin(0.00001f));
    }
}

//========================
TEST_CASE("Hanning window generation matches golden CSV values", "[BufferFiller]")
{
    // Load the golden Hanning window from CSV
    juce::AudioBuffer<float> goldenBuffer;
    goldenBuffer.clear();

    juce::File currentDir = juce::File::getCurrentWorkingDirectory();
    juce::String relativePath = "/SUBMODULES/RD/TESTS/GOLDEN/GOLDEN_HanningBuffer_1024.csv";
    juce::String fullPath = currentDir.getFullPathName() + relativePath;

    bool loadSuccess = BufferFiller::loadFromCSV(goldenBuffer, fullPath);
    REQUIRE(loadSuccess == true);
    REQUIRE(goldenBuffer.getNumSamples() == 1024);

    // Generate a Hanning window using BufferFiller::generateHanning
    juce::AudioBuffer<float> generatedBuffer(1, 1024);
    BufferFiller::generateHanning(generatedBuffer);

    // Compare the two buffers sample by sample
    for(int sampleIndex = 0; sampleIndex < 1024; sampleIndex++)
    {
        float goldenValue = goldenBuffer.getSample(0, sampleIndex);
        float generatedValue = generatedBuffer.getSample(0, sampleIndex);

        INFO("Comparing sample at index " << sampleIndex);
        CHECK(generatedValue == Catch::Approx(goldenValue).margin(0.00001f));
    }

    // Also verify key values every 128 samples
    CHECK(generatedBuffer.getSample(0, 0) == Catch::Approx(0.0f).margin(0.00001f));
    CHECK(generatedBuffer.getSample(0, 128) == Catch::Approx(goldenBuffer.getSample(0, 128)).margin(0.00001f));
    CHECK(generatedBuffer.getSample(0, 256) == Catch::Approx(goldenBuffer.getSample(0, 256)).margin(0.00001f));
    CHECK(generatedBuffer.getSample(0, 512) == Catch::Approx(goldenBuffer.getSample(0, 512)).margin(0.00001f));
    CHECK(generatedBuffer.getSample(0, 768) == Catch::Approx(goldenBuffer.getSample(0, 768)).margin(0.00001f));
    CHECK(generatedBuffer.getSample(0, 896) == Catch::Approx(goldenBuffer.getSample(0, 896)).margin(0.00001f));
    CHECK(generatedBuffer.getSample(0, 1023) == Catch::Approx(0.0f).margin(0.00001f));
}

//========================
TEST_CASE("Can make an incremental buffer", "[BufferFiller]")
{
    int numSamples = 100;
    juce::AudioBuffer<float> buffer(1, numSamples);

    BufferFiller::fillIncremental(buffer);

    // checking values are incremental
    for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
    {
        int sample = (int)buffer.getSample(0, sampleIndex);
        CHECK(sample == sampleIndex );
    }

}

//========================
TEST_CASE("Can make an incremental buffer that loops at the period length arg", "[BufferFiller]")
{
    int numSamples = 100;
	int periodLength = 10;
    juce::AudioBuffer<float> buffer(1, numSamples);

    BufferFiller::fillIncrementalLooping(buffer, periodLength);

	int sampleInPeriod = 0;
    // checking values are incremental
    for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
    {
        int sample = (int)buffer.getSample(0, sampleIndex);
        CHECK(sample == sampleInPeriod );
		CHECK(sample < periodLength);
		
		sampleInPeriod++;
		if(sampleInPeriod >= periodLength)
			sampleInPeriod = sampleInPeriod - periodLength;
    }

}

//=========================
TEST_CASE("Can fill with alternating zeroes and ones", "[BufferFiller]")
{
    int numSamples = 100;
    juce::AudioBuffer<float> buffer(1, numSamples);

    BufferFiller::fillAlternatingZeroOne(buffer);

    CHECK(buffer.getSample(0, 0) == 0.f);
    CHECK(buffer.getSample(0, 1) == 1.f);
    CHECK(buffer.getSample(0, 98) == 0.f);
    CHECK(buffer.getSample(0, 99) == 1.f);
}


//=========================
TEST_CASE("Can fill with specified value", "[BufferFiller]")
{
    int numSamples = 100;
    juce::AudioBuffer<float> buffer(1, numSamples);
    float value = 2.f;
    BufferFiller::fillWithValue(buffer, value);

    CHECK(buffer.getSample(0, 0) == value);
    CHECK(buffer.getSample(0, 1) == value);
    CHECK(buffer.getSample(0, 98) == value);
    CHECK(buffer.getSample(0, 99) == value);
}


//=========================
TEST_CASE("Can fill channel with specified value", "[BufferFiller]")
{
    int numSamples = 100;
    juce::AudioBuffer<float> buffer(2, numSamples);
    int val1 = 1;
    BufferFiller::fillChannelWithValue(buffer, 0, val1);
    int val2 = 2;
    BufferFiller::fillChannelWithValue(buffer, 1, val2);

    CHECK(buffer.getSample(0, 0) == static_cast<float>(val1));
    CHECK(buffer.getSample(0, 1) == static_cast<float>(val1));
    CHECK(buffer.getSample(0, 98) == static_cast<float>(val1));
    CHECK(buffer.getSample(0, 99) == static_cast<float>(val1));


    CHECK(buffer.getSample(1, 0) == val2);
    CHECK(buffer.getSample(1, 1) == val2);
    CHECK(buffer.getSample(1, 98) == val2);
    CHECK(buffer.getSample(1, 99) == val2);
}

//=========================
TEST_CASE("Can fill range with specified value", "[BufferFiller]")
{
    int numSamples = 100;
    juce::AudioBuffer<float> buffer(2, numSamples);
    buffer.clear();

    SECTION("Fill range on all channels (default)")
    {
        float value = 5.f;
        bool result = BufferFiller::fillRangeWithValue(buffer, 10, 20, value);

        CHECK(result == true);

        // Check samples before range are still 0
        CHECK(buffer.getSample(0, 9) == 0.f);
        CHECK(buffer.getSample(1, 9) == 0.f);

        // Check samples in range are filled on both channels
        CHECK(buffer.getSample(0, 10) == value);
        CHECK(buffer.getSample(1, 10) == value);
        CHECK(buffer.getSample(0, 15) == value);
        CHECK(buffer.getSample(1, 15) == value);
        CHECK(buffer.getSample(0, 20) == value);
        CHECK(buffer.getSample(1, 20) == value);

        // Check samples after range are still 0
        CHECK(buffer.getSample(0, 21) == 0.f);
        CHECK(buffer.getSample(1, 21) == 0.f);
    }

    SECTION("Fill range on specific channel only")
    {
        float value = 3.f;
        bool result = BufferFiller::fillRangeWithValue(buffer, 30, 40, value, 0);

        CHECK(result == true);

        // Channel 0 should have values in range
        CHECK(buffer.getSample(0, 30) == value);
        CHECK(buffer.getSample(0, 35) == value);
        CHECK(buffer.getSample(0, 40) == value);

        // Channel 1 should still be 0
        CHECK(buffer.getSample(1, 30) == 0.f);
        CHECK(buffer.getSample(1, 35) == 0.f);
        CHECK(buffer.getSample(1, 40) == 0.f);
    }

    SECTION("Returns false for out-of-range start index")
    {
        bool result = BufferFiller::fillRangeWithValue(buffer, -1, 10, 1.f);
        CHECK(result == false);
    }

    SECTION("Returns false for out-of-range end index")
    {
        bool result = BufferFiller::fillRangeWithValue(buffer, 0, 100, 1.f);
        CHECK(result == false);
    }

    SECTION("Returns false when start > end")
    {
        bool result = BufferFiller::fillRangeWithValue(buffer, 50, 40, 1.f);
        CHECK(result == false);
    }

    SECTION("Returns false for out-of-range channel")
    {
        bool result = BufferFiller::fillRangeWithValue(buffer, 0, 10, 1.f, 5);
        CHECK(result == false);
    }

    SECTION("Returns false for negative channel (other than -1)")
    {
        bool result = BufferFiller::fillRangeWithValue(buffer, 0, 10, 1.f, -2);
        CHECK(result == false);
    }
}

//==========================
TEST_CASE("Test generating sine wave", "[BufferFiller]")
{
	juce::AudioBuffer<float> sineWaveBuffer(1, 128);
	sineWaveBuffer.clear();
	
	BufferFiller::generateSine(sineWaveBuffer);
	auto sineWaveRMS = sineWaveBuffer.getRMSLevel(0, 0, 128);
	auto halfPower = std::sqrt(0.5f); // sine wave rms is equal to this
	auto errorTolerance = 0.0001f;
	REQUIRE(std::abs(sineWaveRMS - halfPower) < errorTolerance);
}

//==========================
TEST_CASE("Test generating sine wave cycles", "[BufferFiller]")
{
    int bufferLength = 1024;
    int period = 256; // sine wave every 256 samples, 4 cycles in a buffer of 1024
	juce::AudioBuffer<float> sineWaveBuffer(1, bufferLength);
	sineWaveBuffer.clear();
	
	BufferFiller::generateSineCycles(sineWaveBuffer, period);

    ///////////////
    // Check RMS
	auto sineWaveRMS = sineWaveBuffer.getRMSLevel(0, 0, bufferLength);
	auto halfPower = std::sqrt(0.5f); // sine wave rms is equal to this
	auto errorTolerance = 0.0001f;
	REQUIRE(std::abs(sineWaveRMS - halfPower) < errorTolerance);

    //////////////
    // Check for equality at each index + period.  Start at one to make sample index test easier, not loosing coverage
    for(int sampleIndex = 0; sampleIndex < period; sampleIndex++)
    {
        // cycle 1/2/3/4 respectively
        float sample1 = sineWaveBuffer.getSample(0, sampleIndex );
        float sample2 = sineWaveBuffer.getSample(0, sampleIndex + (period) );
        [[maybe_unused]] float sample3 = sineWaveBuffer.getSample(0, sampleIndex + (period * 2) );
        [[maybe_unused]] float sample4 = sineWaveBuffer.getSample(0, sampleIndex + (period * 3) );
        CHECK(sample1 == sample2);
    }
}




//==========================
// This test can fail if you run the test exectuable by clicking on it in finder
// run from root of plugin project instead by calling BUILD/Tests
TEST_CASE("Can load a json file into a buffer", "[BufferFiller]")
{
    juce::AudioBuffer<float> buffer;
    juce::File currentDir = juce::File::getCurrentWorkingDirectory(); // this works when called from root dir of repo
    juce::String relativePath = "/SUBMODULES//RD/TESTS/GOLDEN/gold_incremental.json"; 

    juce::String fullPath = currentDir.getFullPathName() + relativePath;
    // Instantiate the juce::File using the relative path
    juce::File file(fullPath);

    BufferFiller::loadFromJsonFile(file, buffer, "Incremental");

    CHECK(buffer.getNumSamples() > 10);


    // Test reading ability with incremental buffer.  The int version of the stored sample value should match its sample index
    for(int ch = 0; ch < buffer.getNumChannels(); ch++)
    {
        for(int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); sampleIndex++)
        {
            int sample = (int)buffer.getSample(ch, sampleIndex);
            CHECK(sample == sampleIndex);
        }
    }
}



//==========================
TEST_CASE("Load .wav into buffer", "[BufferFiller]")
{
    /**
     * @brief Load two different golden wav files into buffers, then compare them.
     * Also, we expect them both to have an rms of more than 0
     * 
     */
    juce::AudioBuffer<float> somewhereBuffer;
    juce::AudioBuffer<float> graceBuffer;


    auto somewhereFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
    auto graceFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_AmazingGrace_Mono_441k.wav");

    BufferFiller::loadFromWavFile(somewhereFile, somewhereBuffer);
    BufferFiller::loadFromWavFile(graceFile, graceBuffer);

    CHECK(somewhereBuffer.getRMSLevel(0, 0, somewhereBuffer.getNumSamples() - 1) > 0.0);
    CHECK(graceBuffer.getRMSLevel(0, 0, somewhereBuffer.getNumSamples() - 1) > 0.0);

    bool areIdentical = BufferHelper::buffersAreIdentical(somewhereBuffer, graceBuffer);
    CHECK(!areIdentical);

}




//===============================
TEST_CASE("Can load a csv file into a buffer", "[BufferFiller]")
{
    juce::AudioBuffer<float> buffer;
    buffer.clear();
    juce::File currentDir = juce::File::getCurrentWorkingDirectory(); // this works when called from root dir of repo
    juce::String relativePath = "/SUBMODULES//RD/TESTS/GOLDEN/GOLDEN_stereo_incremental_buffer.csv"; 

    juce::String fullPath = currentDir.getFullPathName() + relativePath;

    buffer.setSize(1, 1); // this will be undone by the loading of the stereo csv with 6 samples
    bool result = BufferFiller::loadFromCSV(buffer, fullPath);

    // Should succeed in loading, and numChannels/numSamples should have changed from (1,1)
    CHECK(result == true);
    CHECK(buffer.getNumChannels() == 2);
    CHECK(buffer.getNumSamples() == 6);


    // Test reading ability with incremental buffer.  The int version of the stored sample value should match its sample index
    for(int ch = 0; ch < buffer.getNumChannels(); ch++)
    {
        for(int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); sampleIndex++)
        {
            int sample = (int)buffer.getSample(ch, sampleIndex);
            CHECK(sample == sampleIndex);
        }
    }
}




//========================
TEST_CASE("Load juce::Array into buffer", "[BufferFiller]")
{
    juce::Array<juce::Array<float>> array;

    array.add(juce::Array<float>({0.1f, 0.2f, 0.3f, 0.4f, 0.5f}));
    array.add(juce::Array<float>({0.1f, 0.2f, 0.3f, 0.4f, 0.5f}));


    juce::AudioBuffer<float> buffer;
    buffer.clear();
    buffer.setSize(1, 20);  // this should get changed by the fill function

    BufferFiller::fillWithJuceArray(buffer, array);

    CHECK(buffer.getNumChannels() == 2);
    CHECK(buffer.getNumSamples() == 5);

    for(int ch = 0; ch < 2; ch++)
    {
        for(int sampleIndex = 0; sampleIndex < 5; sampleIndex++)
        {
            auto sampleVal = buffer.getSample(ch, sampleIndex);
            float expectedVal = (float)(sampleIndex + 1) * 0.1f;
            CHECK(sampleVal == expectedVal);
        }
    }


}

