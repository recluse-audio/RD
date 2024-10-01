#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+
#include "../SOURCE/BufferFiller.h"
#include "../SOURCE/BufferWriter.h"

// Being Tested
#include "../SOURCE/BufferMath.h"

TEST_CASE("Difference function can be applied")
{
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



    auto jsonName = BufferWriter::getTestOutputPath("expect_2cycle_sine_wave.json");
    juce::File jsonFile(jsonName);
    BufferWriter::writeToJson(ioBuffer, jsonFile);

    auto diffJsonPath = BufferWriter::getTestOutputPath("failing_difference_buffer.json");
    juce::File diffJsonFile(diffJsonPath);
    BufferWriter::writeToJson(differenceBuffer, diffJsonFile);


    // minimal difference at half buffer size and 0 lag, aka perfectly in phase
    CHECK( differenceBuffer.getSample(0, 0) == Catch::Approx(0.0f).margin(1e-4f) );

    // this is the sample index 1/4 through difference buffer, aka pi radianos or 90 degree phase
    int sampleIndexOf180DegreePhase = (diffBufferSize / 2); // [255] in diffBuffer of size 512
    CHECK( differenceBuffer.getSample(0, sampleIndexOf180DegreePhase) == Catch::Approx((float)ioBufferSize).margin(1e-4f) );



    // // halfway through the period, aka diffBufferSize, this represents 180 through the phase of the sine wave.
    // int quarterBufferSize = diffBufferSize / 2; 
    // // This is 180 degrees out of phase, should be max difference
    // float expectedMaxDifference = differenceBuffer.getSample(0, quarterBufferSize);
    // // loop through differenceBuffer, make sure lag of 256 is max difference.  
    // for(int diffIndex = 0; diffIndex < diffBufferSize; diffIndex++)
    // {
    //     // don't count the expected max.  No, '<=' won't work in the checks for the most hypothetical of situations
    //     // what if some other index some how had exactly the same difference as the expectedMaxDifference and it shouldn't?
    //     // this justifies the if() bailout method
    //     if(diffIndex == quarterBufferSize)
    //         continue;

    //     float differenceToCompare = differenceBuffer.getSample(0, diffIndex);
    //     CHECK(differenceToCompare < expectedMaxDifference);
    // }
}