#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+

#include "../SOURCE/AudioFileProcessor.h"
#include "../SOURCE/RelativeFilePath.h"
#include "../SOURCE/BufferFiller.h"

TEST_CASE("Can read/write .wav without processing.")
{
    AudioFileProcessor fileProcessor;

    auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
    auto outputFile = RelativeFilePath::getOutputFileFromProjectRoot("Somewhere_Mono_441k_Transposed.wav");
    bool success = fileProcessor.init(inputFile, outputFile);
    // AudioFileProcessor should setup reader/writer/stream and return true
    CHECK(success);

    // Pre-known value for golden file "Somewhere_Mono_441k.wav"
    // so we don't have to do a whole .wav file reader instantiation and such.
    int inputFileNumSamples = 1531887;
    int numChannels = 1; // mono per name of golden file

    // AudioFileProcessor should update internal mTotalSamples correctly
    CHECK(fileProcessor.getNumReadSamples() == inputFileNumSamples);

    int numSamples = 512;
    // buffer to read from inputFile and write to outputFile
    juce::AudioBuffer<float> buffer(numChannels, numSamples);
    buffer.clear();

    fileProcessor.read(buffer, 88200);

    // filling buffer from golden json
    juce::AudioBuffer<float> goldenBuffer(numChannels, numSamples);
    auto goldenJsonFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k_Two_Second_Mark.json");
    BufferFiller::loadFromJsonFile(goldenJsonFile, goldenBuffer);

    for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
    {

        for(int ch = 0; ch < numChannels; ch++)
        {
            auto sampleToCheck = buffer.getSample(ch, sampleIndex);
            auto goldenSample = goldenBuffer.getSample(ch, sampleIndex);
            // CHECK(sampleToCheck != 0.f);
            // CHECK(goldenSample != 0.f);
            CHECK(sampleToCheck == goldenSample); // TODO: this is a temp test, sometimes it could be 0
        }
    }

    // // do processing on buffer here
    // fileProcessor.write(buffer);

}