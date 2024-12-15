#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+

#include "../SOURCE/RelativeFilePath.h"
#include "../SOURCE/BufferFiller.h"

TEST_CASE("Can get file relative to project root directory.")
{
    juce::String relativePath("/SUBMODULES/RD/TESTS/GOLDEN/gold_incremental.json");
    auto file = RelativeFilePath::getFileFromProjectRoot(relativePath);

    juce::AudioBuffer<float> buffer(1, 128);
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

TEST_CASE("Can get golden file directly.")
{
    juce::String relativePath("gold_incremental.json");
    auto file = RelativeFilePath::getGoldenFileFromProjectRoot(relativePath);

    juce::AudioBuffer<float> buffer(1, 128);
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

TEST_CASE("Can create empty output file")
{
    juce::String outputPath = "Test_Output.txt";
    auto file = RelativeFilePath::getOutputFileFromProjectRoot(outputPath);
    file.appendText("text for testing");
    CHECK(file.existsAsFile());

}