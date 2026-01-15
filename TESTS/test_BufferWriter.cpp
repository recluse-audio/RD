#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+
#include "../SOURCE/BufferFiller.h"

#include "../SOURCE/BufferWriter.h"
#include "../SOURCE/BufferHelper.h"

#include "../SOURCE/RelativeFilePath.h"



//////// More so testing the buffer filler options

TEST_CASE("Can write sine wave to json")
{
    juce::AudioBuffer<float> buffer(1, 128);
    BufferFiller::generateSine(buffer);

    auto jsonName = BufferWriter::getTestOutputPath("sine_wave_json.json");
    juce::File jsonFile(jsonName);
    BufferWriter::writeToJson(buffer, jsonFile);
}



TEST_CASE("Can write sine wave to csv")
{
    juce::AudioBuffer<float> buffer(1, 128);
    BufferFiller::generateSine(buffer);

    auto csvName = BufferWriter::getTestOutputPath("sine_wave_csv.csv");
    juce::File csvFile(csvName);
    BufferWriter::writeToJson(buffer, csvFile);
}







TEST_CASE("Can write buffer to json")
{
    juce::AudioBuffer<float> buffer(1, 128);
    BufferFiller::fillIncremental(buffer);

    auto jsonName = BufferWriter::getTestOutputPath("test_incremental.json");
    juce::File jsonFile(jsonName);
    BufferWriter::writeToJson(buffer, jsonFile);
}



TEST_CASE("Can write buffer to csv")
{
    // See test_BufferFiller.cpp for more extensive coverage of this as I test
    // filling a buffer from a csv file.
    juce::AudioBuffer<float> buffer(2, 128);
    BufferFiller::fillIncremental(buffer);

    auto testOutputPath = RelativeFilePath::getTestOutputPath("OUTPUT_mono_incremental_buffer.csv");
    auto result = BufferWriter::writeToCSV(buffer, testOutputPath);

    CHECK(result == BufferWriter::Result::kSuccess);


    juce::AudioBuffer<float> testBuffer;
    juce::AudioBuffer<float> goldenBuffer;

    testBuffer.clear(); goldenBuffer.clear();

    // load test buffer from test output
    BufferFiller::loadFromCSV(testBuffer, testOutputPath);

    // load golden buffer from golden csv
    auto goldenPath = RelativeFilePath::getGoldenFilePath("GOLDEN_mono_incremental_buffer.csv");
    BufferFiller::loadFromCSV(goldenBuffer, goldenPath);

    bool areIdentical = BufferHelper::buffersAreIdentical(testBuffer, goldenBuffer);
    CHECK(areIdentical);
}


TEST_CASE("Can load buffer from .wav and write to csv.")
{
    // See test_BufferFiller.cpp for more extensive coverage of this as I test
    // filling a buffer from a csv file.
    juce::AudioBuffer<float> buffer(2, 128);

    auto goldenWavFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_SignalTone.wav");
    BufferFiller::loadFromWavFile(goldenWavFile, buffer);

    auto testOutputPath = RelativeFilePath::getTestOutputPath("OUTPUT_SignalTone.csv");
    [[maybe_unused]] auto result = BufferWriter::writeToCSV(buffer, testOutputPath);

}


TEST_CASE("Can write buffer to .wav.")
{
    juce::AudioBuffer<float> buffer;
    juce::AudioBuffer<float> buffer2;

    auto goldenWavFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_SignalTone.wav");
    BufferFiller::loadFromWavFile(goldenWavFile, buffer);
    BufferFiller::loadFromWavFile(goldenWavFile, buffer2);
    bool testIdentical = BufferHelper::buffersAreIdentical(buffer, buffer2);
    CHECK(testIdentical);

    auto outputWavFile = RelativeFilePath::getOutputFileFromProjectRoot("OUTPUT_Write_SignalTone_Buffer_To_Wav.wav");
    auto result = BufferWriter::writeToWav(buffer, outputWavFile, 44100, 24);

    CHECK(result == BufferWriter::Result::kSuccess);

    juce::AudioBuffer<float> outputBuffer;
    outputBuffer.clear();
    outputBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples());

    // auto somewhereFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav"); // used this to make test fail originally
    BufferFiller::loadFromWavFile(outputWavFile, outputBuffer);

    CHECK(!BufferHelper::isSilent(buffer));
    CHECK(!BufferHelper::isSilent(outputBuffer));

    bool areIdentical = BufferHelper::buffersAreIdentical(buffer, outputBuffer);
    CHECK(areIdentical);

}