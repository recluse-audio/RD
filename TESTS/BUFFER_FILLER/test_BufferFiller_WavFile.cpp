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

    BufferFiller::loadFromWavFile(somewhereFile, somewhereBuffer);
    BufferFiller::loadFromWavFile(graceFile, graceBuffer);

    CHECK(somewhereBuffer.getRMSLevel(0, 0, somewhereBuffer.getNumSamples() - 1) > 0.0);
    CHECK(graceBuffer.getRMSLevel(0, 0, somewhereBuffer.getNumSamples() - 1) > 0.0);

    bool areIdentical = BufferHelper::buffersAreIdentical(somewhereBuffer, graceBuffer);
    CHECK(!areIdentical);
}
