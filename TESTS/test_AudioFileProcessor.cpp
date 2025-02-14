#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+

#include "../SOURCE/AudioFileProcessor.h"
#include "../SOURCE/RelativeFilePath.h"
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/BufferHelper.h"
#include "../SOURCE/AudioFileHelpers.h"


static const int kGoldenNumSamples = 1531887;
static const int kGoldenNumChannels = 1;
static const int kGoldenBufferNumSamples = 512;
static const int kFileStartIndex = 88200;

//
TEST_CASE("Can read .wav")
{
    AudioFileProcessor fileProcessor;

    auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
    auto outputFile = RelativeFilePath::getOutputFileFromProjectRoot("Somewhere_Mono_441k_Transposed.wav");
    bool success = fileProcessor.init(inputFile, outputFile);
    // AudioFileProcessor should setup reader/writer/stream and return true
    CHECK(success);


    // AudioFileProcessor should update internal mTotalSamples correctly
    CHECK(fileProcessor.getNumReadSamples() == kGoldenNumSamples);

    // buffer to read from inputFile and write to outputFile
    juce::AudioBuffer<float> buffer(kGoldenNumChannels, kGoldenBufferNumSamples);
    buffer.clear();

    fileProcessor.read(buffer, kFileStartIndex);

    // filling buffer from golden json
    juce::AudioBuffer<float> goldenBuffer(kGoldenNumChannels, kGoldenBufferNumSamples);
    auto goldenJsonFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k_Two_Second_Mark.json");
    BufferFiller::loadFromJsonFile(goldenJsonFile, goldenBuffer);

    // check reading a section of golden wav using read() method agains golden json of chunk of 512 samples in wav file
    bool matchesGolden = BufferHelper::buffersAreIdentical(buffer, goldenBuffer);
    CHECK(matchesGolden);


}



//
TEST_CASE("Basic read/write")
{
    AudioFileProcessor fileProcessor;

    auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
    auto outputFile = RelativeFilePath::getOutputFileFromProjectRoot("AudioFileProcessor_Basic_Read_Write_Test_Output.wav");
    bool success = fileProcessor.init(inputFile, outputFile);

    // buffer to read from inputFile and write to outputFile
    juce::AudioBuffer<float> processBuffer(kGoldenNumChannels, kGoldenBufferNumSamples);
    processBuffer.clear();

    int currentIndex = 0;
    while(currentIndex < (kGoldenNumSamples - 512))
    {
        fileProcessor.read(processBuffer, currentIndex);
        fileProcessor.write(processBuffer, currentIndex);
        currentIndex += processBuffer.getNumSamples();
    }


    // use this buffer to read from our new .wav file
    juce::AudioBuffer<float> testBuffer(kGoldenNumChannels, kGoldenBufferNumSamples);
    testBuffer.clear();

    AudioFileHelpers::readRange(outputFile, testBuffer, kFileStartIndex);

    CHECK(BufferHelper::buffersAreIdentical(processBuffer, testBuffer));


}