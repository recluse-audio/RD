#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+
#include "../SOURCE/BufferFiller.h"

#include "../SOURCE/BufferWriter.h"



//////// More so testing the buffer filler options

TEST_CASE("Can write sine wave to json")
{
    juce::AudioBuffer<float> buffer(1, 128);
    BufferFiller::generateSine(buffer);

    auto jsonName = BufferWriter::getTestOutputPath("sine_wave_json.json");
    juce::File jsonFile(jsonName);
    BufferWriter::writeToJson(buffer, jsonFile);
}











TEST_CASE("Can write buffer to json")
{
    juce::AudioBuffer<float> buffer(1, 128);
    BufferFiller::fillIncremental(buffer);

    auto jsonName = BufferWriter::getTestOutputPath("test_incremental.json");
    juce::File jsonFile(jsonName);
    BufferWriter::writeToJson(buffer, jsonFile);
}


