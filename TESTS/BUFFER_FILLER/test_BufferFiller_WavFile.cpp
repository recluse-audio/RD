#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>
#include "../../SOURCE/BUFFER_FILLER/BufferFiller.h"
#include "../../SOURCE/BufferHelper.h"
#include "../../SOURCE/RelativeFilePath.h"


//==========================
TEST_CASE("Load .wav into buffer", "[BufferFiller][.wav]")
{
    /**
     * @brief Load two different golden wav files into buffers, then compare them.
     * Also, we expect them both to have an rms of more than 0
     */
    juce::AudioBuffer<float> somewhereBuffer;
    juce::AudioBuffer<float> graceBuffer;

    auto somewhereFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
    auto graceFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_AmazingGrace_Mono_441k.wav");

    const int initialNumSamples = somewhereBuffer.getNumSamples();
    const int initialNumChannels = somewhereBuffer.getNumChannels();

    BufferFiller::loadFromWavFile(somewhereFile, somewhereBuffer);
    BufferFiller::loadFromWavFile(graceFile, graceBuffer);

    // loadFromWavFile must resize buffer to file dimensions
    CHECK(somewhereBuffer.getNumSamples() != initialNumSamples);
    CHECK(somewhereBuffer.getNumSamples() > 0);
    CHECK(somewhereBuffer.getNumChannels() > 0);
    CHECK((somewhereBuffer.getNumSamples() != initialNumSamples
           || somewhereBuffer.getNumChannels() != initialNumChannels));

    CHECK(somewhereBuffer.getRMSLevel(0, 0, somewhereBuffer.getNumSamples() - 1) > 0.0);
    CHECK(graceBuffer.getRMSLevel(0, 0, somewhereBuffer.getNumSamples() - 1) > 0.0);

    bool areIdentical = BufferHelper::buffersAreIdentical(somewhereBuffer, graceBuffer);
    CHECK(!areIdentical);
}

//==========================
TEST_CASE("Fill .wav into pre-sized buffer", "[BufferFiller][.wav]")
{
    /**
     * @brief fillFromWavFile must NOT resize the destination buffer.
     * Pre-size buffer, fill from two different golden files, confirm size unchanged
     * and contents differ / have non-zero RMS.
     */
    constexpr int kPreChannels = 1;
    constexpr int kPreSamples  = 4096;

    juce::AudioBuffer<float> somewhereBuffer(kPreChannels, kPreSamples);
    juce::AudioBuffer<float> graceBuffer(kPreChannels, kPreSamples);
    somewhereBuffer.clear();
    graceBuffer.clear();

    auto somewhereFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
    auto graceFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_AmazingGrace_Mono_441k.wav");

    REQUIRE(BufferFiller::fillFromWavFile(somewhereFile, somewhereBuffer));
    REQUIRE(BufferFiller::fillFromWavFile(graceFile, graceBuffer));

    // fillFromWavFile must NOT resize the buffer
    CHECK(somewhereBuffer.getNumSamples() == kPreSamples);
    CHECK(somewhereBuffer.getNumChannels() == kPreChannels);
    CHECK(graceBuffer.getNumSamples() == kPreSamples);
    CHECK(graceBuffer.getNumChannels() == kPreChannels);

    CHECK(somewhereBuffer.getRMSLevel(0, 0, somewhereBuffer.getNumSamples() - 1) > 0.0);
    CHECK(graceBuffer.getRMSLevel(0, 0, graceBuffer.getNumSamples() - 1) > 0.0);

    bool areIdentical = BufferHelper::buffersAreIdentical(somewhereBuffer, graceBuffer);
    CHECK(!areIdentical);
}
