#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+

#include "../SOURCE/AudioFileProcessor.h"
#include "../SOURCE/RelativeFilePath.h"
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/BufferHelper.h"

static const int kGoldenNumSamples = 1531887;

TEST_CASE("Can read/write .wav without processing.")
{
    AudioFileProcessor fileProcessor;

    auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
    auto outputFile = RelativeFilePath::getOutputFileFromProjectRoot("Somewhere_Mono_441k_Transposed.wav");
    bool success = fileProcessor.init(inputFile, outputFile);
    // AudioFileProcessor should setup reader/writer/stream and return true
    CHECK(success);

    // Pre-known value for golden file "Somewhere_Mono_441k.wav"
    int numChannels = 1; // mono per name of golden file

    // AudioFileProcessor should update internal mTotalSamples correctly
    CHECK(fileProcessor.getNumReadSamples() == kGoldenNumSamples);

    int numSamples = 512;
    // buffer to read from inputFile and write to outputFile
    juce::AudioBuffer<float> buffer(numChannels, numSamples);
    buffer.clear();

    fileProcessor.read(buffer, 88200);

    // filling buffer from golden json
    juce::AudioBuffer<float> goldenBuffer(numChannels, numSamples);
    auto goldenJsonFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k_Two_Second_Mark.json");
    BufferFiller::loadFromJsonFile(goldenJsonFile, goldenBuffer);

    // check reading a section of golden wav using read() method agains golden json of chunk of 512 samples in wav file
    bool matchesGolden = BufferHelper::buffersAreIdentical(buffer, goldenBuffer);
    CHECK(matchesGolden);


}