#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+

#include "../SOURCE/RelativeFilePath.h"
#include "../SOURCE/AudioFileHelpers.h"

#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/BufferWriter.h"


//===============================
//
TEST_CASE("Read range from wav file")
{
    int numChannels = 1;
    int numSamples = 512;

    auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
    juce::AudioBuffer<float> outputBuffer(numChannels, numSamples);
    outputBuffer.clear();

    int startSample = 88200;

    AudioFileHelpers::readRange(inputFile, outputBuffer, startSample);

    // filling from golden json
    juce::AudioBuffer<float> goldenBuffer(numChannels, numSamples);
    auto goldenJsonFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k_Two_Second_Mark.json");
    BufferFiller::loadFromJsonFile(goldenJsonFile, goldenBuffer);

    for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
    {

        for(int ch = 0; ch < numChannels; ch++)
        {
            auto sampleToCheck = outputBuffer.getSample(ch, sampleIndex);
            auto goldenSample = goldenBuffer.getSample(ch, sampleIndex);
            CHECK(sampleToCheck == goldenSample); // TODO: this is a temp test, sometimes it could be 0
        }
    }

}

//===============================
// 
TEST_CASE("Get length in samples of wav file.")
{
    auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
    int lengthInSamples = AudioFileHelpers::getFileLengthInSamples(inputFile);

    CHECK(lengthInSamples == 1531887);
}

//===============================
//
TEST_CASE("Get sampleRate of wav file.")
{
    auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
    int sampleRate = static_cast<int>(AudioFileHelpers::getFileSampleRate(inputFile));

    CHECK(sampleRate == 44100);
}
//===============================
//
TEST_CASE("Create GOLDEN_Somewhere_Mono_441k_Two_Second_Mark.json for further testing")
{
    // int numChannels = 1;
    // int numSamples = 512;

    // auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
    // juce::AudioBuffer<float> outputBuffer(numChannels, numSamples);
    // outputBuffer.clear();

    // int startSample = 88200;

    // AudioFileHelpers::readRange(inputFile, outputBuffer, startSample);

    // for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
    // {

    //     for(int ch = 0; ch < numChannels; ch++)
    //     {
    //         auto sampleToCheck = outputBuffer.getSample(ch, sampleIndex);
    //         CHECK(sampleToCheck != 0); // TODO: this is a temp test, sometimes it could be 0
    //     }
    // }

    // auto outputFile = RelativeFilePath::getOutputFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k_Two_Second_Mark.json");

    // BufferWriter::writeToJson(outputBuffer, outputFile);
}