// TODO, move this to a common or DSP_TOOLS test folder

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+
#include "../SOURCE/BufferFiller.h"


TEST_CASE("Can make a Hanning Window")
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

        CHECK(sample1 == Catch::Approx(sample2).epsilon(0.0001f));
    }
}

//========================
TEST_CASE("Can make an incremental buffer")
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

//=========================
TEST_CASE("Can fill with alternating zeroes and ones")
{
    int numSamples = 100;
    juce::AudioBuffer<float> buffer(1, numSamples);

    BufferFiller::fillAlternatingZeroOne(buffer);

    CHECK(buffer.getSample(0, 0) == 0.f);
    CHECK(buffer.getSample(0, 1) == 1.f);
    CHECK(buffer.getSample(0, 98) == 0.f);
    CHECK(buffer.getSample(0, 99) == 1.f);
}


//==========================
TEST_CASE("Can load a wav file into a buffer")
{
    juce::AudioBuffer<float> buffer;

    // Relative to Catch2 executable in BUILD folder
    juce::String relativePath = "../SUBMODULES//RD/WAVEFORMS/incremental_wave.wav"; 

    // Instantiate the juce::File using the relative path
    juce::File file(relativePath);

    // Check if the file exists
    if (file.existsAsFile())
    {
        DBG("File exists at: " + file.getFullPathName());
    }
    else
    {
        DBG("File does not exist at: " + file.getFullPathName());
    }

    BufferFiller::loadFromWavFile(file.getFullPathName(), buffer);

    // Test reading ability with incremental buffer
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
TEST_CASE("Can load a json file into a buffer")
{
    juce::AudioBuffer<float> buffer;

    juce::File currentDir = juce::File::getCurrentWorkingDirectory();
    DBG("Current Working Directory: " + currentDir.getFullPathName());

    // Relative to Catch2 executable in BUILD folder
    juce::String relativePath = "/SUBMODULES//RD/WAVEFORMS/incremental_wave.json"; 

    // Instantiate the juce::File using the relative path
    juce::File file(relativePath);

    CHECK(file.existsAsFile());

    // Check if the file exists
    if (file.existsAsFile())
    {
        DBG("File exists at: " + file.getFullPathName());
    }
    else
    {
        DBG("File does not exist at: " + file.getFullPathName());
    }

    BufferFiller::loadFromJsonFile(file.getFullPathName(), buffer);

    CHECK(buffer.getNumSamples() > 10);
    // Test reading ability with incremental buffer
    for(int ch = 0; ch < buffer.getNumChannels(); ch++)
    {
        for(int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); sampleIndex++)
        {
            int sample = (int)buffer.getSample(ch, sampleIndex);
            CHECK(sample == sampleIndex);
        }
    }
}