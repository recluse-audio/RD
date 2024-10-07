#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/BufferWriter.h"

// Being Tested
#include "../SOURCE/BufferMath.h"

TEST_CASE("Yin By Hand")
{
    //======================
    // MAKE SINE WAVES (2 CYCLES)
    juce::AudioBuffer<float> ioBuffer(1, 32);
    BufferFiller::generateSineCycles(ioBuffer, 16);

    auto sinePath = BufferWriter::getTestOutputPath("handmade_sine.json");
    juce::File sineJson(sinePath);
    BufferWriter::writeToJson(ioBuffer, sineJson);

    //====================
    // DIFFERENCE FUNCTION

    // This buffer holds the calculated difference of the signal 
    // when compared with itself at different lag times aka 'tau'
    juce::AudioBuffer<float> differenceBuffer(1, 16);
    differenceBuffer.clear();

    ////////////////////////
    // Finally the test at hand, yin_difference
    BufferMath::yin_difference(ioBuffer, differenceBuffer, 16);

    auto diffPath = BufferWriter::getTestOutputPath("handmade_diff.json");
    juce::File diffJson(diffPath);
    BufferWriter::writeToJson(differenceBuffer, diffJson);

    //=======================
    // CUMULATIVE MEAN NORMALIZED DIFFERENCE
    juce::AudioBuffer<float> cmndBuffer(differenceBuffer.getNumChannels(), differenceBuffer.getNumSamples());
    cmndBuffer.clear();

    // perform the cumulative mean normalized difference function on the diffBuffer.
    // fills the cmndBuffer with results
    BufferMath::yin_normalized_difference(differenceBuffer, cmndBuffer);


    auto cmndJsonPath = BufferWriter::getTestOutputPath("handmade_cmnd.json");
    juce::File cmndJsonFile(cmndJsonPath);
    BufferWriter::writeToJson(cmndBuffer, cmndJsonFile);
}



TEST_CASE("Difference function can be applied")
{
    bool writeJson = false; // useful debug tool

    // Testing difference function on a buffer that consists of two consecutive sine waves exactly
    int ioBufferSize = 2048; // keep power of two
    int diffBufferSize = ioBufferSize / 2; // utmost of typo caution here

    // This will be filled with EXACTLY two sine waves, who's period is exactly 1/2 the full buffer size
    // This will allow for difference calculation with expected values
    // pretend this is a processBlock() coming in with a sine wave signal that is exactly 2 cycles long
    juce::AudioBuffer<float> ioBuffer(1, ioBufferSize);

    // passing diffBufferSize as period to result in exactly two cycles
    BufferFiller::generateSineCycles(ioBuffer, diffBufferSize);

    // This buffer holds the calculated difference of the signal 
    // when compared with itself at different lag times aka 'tau'
    juce::AudioBuffer<float> differenceBuffer(1, diffBufferSize);
    differenceBuffer.clear();

    ////////////////////////
    // Finally the test at hand, yin_difference
    BufferMath::yin_difference(ioBuffer, differenceBuffer, diffBufferSize);


    //=============
    // write to json if true, this can reveal things
    if(writeJson)
    {
        auto jsonName = BufferWriter::getTestOutputPath("expect_2cycle_sine_wave.json");
        juce::File jsonFile(jsonName);
        BufferWriter::writeToJson(ioBuffer, jsonFile);

        auto diffJsonPath = BufferWriter::getTestOutputPath("difference_buffer.json");
        juce::File diffJsonFile(diffJsonPath);
        BufferWriter::writeToJson(differenceBuffer, diffJsonFile);
    }



    // minimal difference at half buffer size and 0 lag, aka perfectly in phase
    CHECK( differenceBuffer.getSample(0, 0) == Catch::Approx(0.0f).margin(1e-4f) );

    // this is the sample index 1/4 through difference buffer, aka pi radianos or 90 degree phase
    int sampleIndexOf180DegreePhase = (diffBufferSize / 2); // [255] in diffBuffer of size 512
    CHECK( differenceBuffer.getSample(0, sampleIndexOf180DegreePhase) == Catch::Approx((float)ioBufferSize).margin(1e-4f) );


}


//============================
//
TEST_CASE("Cumulative Mean Normalized Difference function can be applied")
{
    bool writeJson = false; // useful debug tool
    
    // Example input signal using JUCE AudioBuffer
    juce::AudioBuffer<float> diffBuffer(1, 6);
    diffBuffer.setSample(0, 0, 0.0f);    // tau = 0 (always 0)
    diffBuffer.setSample(0, 1, 0.05f);   // tau = 1
    diffBuffer.setSample(0, 2, 0.15f);   // tau = 2
    diffBuffer.setSample(0, 3, 0.25f);   // tau = 3
    diffBuffer.setSample(0, 4, 0.35f);   // tau = 4
    diffBuffer.setSample(0, 5, 0.45f);   // tau = 5

    // Buffer filled with the difference values found in two identical sine waves,  
    // auto diffPath = BufferFiller::getGoldenFilePath("gold_1024_difference_buffer.json");
    // juce::File diffJson(diffPath);
    // BufferFiller::loadFromJsonFile(diffJson, diffBuffer); // populates and resizes diffBuffer
    // We've loaded our golden difference json into a buffer at this point.

    // this will be filled shortly.  MUST MATCH diffBuffer!!!
    juce::AudioBuffer<float> cmndBuffer(diffBuffer.getNumChannels(), diffBuffer.getNumSamples());
    cmndBuffer.clear();

    // perform the cumulative mean normalized difference function on the diffBuffer.
    // fills the cmndBuffer with results
    BufferMath::yin_normalized_difference(diffBuffer, cmndBuffer);

    //===================
    if(writeJson)
    {
        auto cmndJsonPath = BufferWriter::getTestOutputPath("cumulative_mean_normalized_diff_buffer.json");
        juce::File cmndJsonFile(cmndJsonPath);
        BufferWriter::writeToJson(cmndBuffer, cmndJsonFile);
    }


    // These are calculated by hand.  Some predictable resulting values, albeit unrealistic
    // CHECK(cmndBuffer.getSample(0, 0) == Catch::Approx(1.f).margin(1e-4));
    // CHECK(cmndBuffer.getSample(0, 1) == Catch::Approx(1.f).margin(1e-4));
    // CHECK(cmndBuffer.getSample(0, 2) == Catch::Approx(0.75f).margin(1e-4));
    // CHECK(cmndBuffer.getSample(0, 3) == Catch::Approx(0.6f).margin(1e-4));
    // CHECK(cmndBuffer.getSample(0, 4) == Catch::Approx(0.525f).margin(1e-4));
    // CHECK(cmndBuffer.getSample(0, 5) == Catch::Approx(0.475f).margin(1e-4));

    // // passing cmndBufferSize as period to result in exactly two cycle
    // BufferFiller::generateSineCycles(ioBuffer, bufferSize);

    // // This buffer holds the calculated difference of the signal 
    // // when compared with itself at different lag times aka 'tau'
    // juce::AudioBuffer<float> differenceBuffer(1, diffBufferSize);
    // differenceBuffer.clear();

    // ////////////////////////
    // // Finally the test at hand, yin_difference
    // BufferMath::yin_difference(ioBuffer, differenceBuffer, diffBufferSize);



    // auto jsonName = BufferWriter::getTestOutputPath("expect_2cycle_sine_wave.json");
    // juce::File jsonFile(jsonName);
    // BufferWriter::writeToJson(ioBuffer, jsonFile);

    // auto cmndJsonPath = BufferWriter::getTestOutputPath("cmnd_buffer.json");
    // juce::File diffJsonFile(diffJsonPath);
    // BufferWriter::writeToJson(differenceBuffer, diffJsonFile);


    // // minimal difference at half buffer size and 0 lag, aka perfectly in phase
    // CHECK( differenceBuffer.getSample(0, 0) == Catch::Approx(0.0f).margin(1e-4f) );

    // // this is the sample index 1/4 through difference buffer, aka pi radianos or 90 degree phase
    // int sampleIndexOf180DegreePhase = (diffBufferSize / 2); // [255] in diffBuffer of size 512
    // CHECK( differenceBuffer.getSample(0, sampleIndexOf180DegreePhase) == Catch::Approx((float)ioBufferSize).margin(1e-4f) );


}


//=======================
//
TEST_CASE("Can find shortest tau under threshold")
{
    juce::AudioBuffer<float> buffer(1, 10);
    buffer.clear();

    buffer.setSample(0, 0, 1.f);
    buffer.setSample(0, 1, 0.1f);
    buffer.setSample(0, 2, 0.1f);
    buffer.setSample(0, 3, 0.2f);
    buffer.setSample(0, 4, 0.1f);
    buffer.setSample(0, 5, 0.1f);
    buffer.setSample(0, 6, 0.1f);
    buffer.setSample(0, 7, 0.1f);
    buffer.setSample(0, 8, 0.1f);
    buffer.setSample(0, 9, 0.1f);

}