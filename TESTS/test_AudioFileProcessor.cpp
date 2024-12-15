#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+

#include "../SOURCE/AudioFileProcessor.h"
#include "../SOURCE/RelativeFilePath.h"

TEST_CASE("Can read/write .wav without processing.")
{
    AudioFileProcessor fileProcessor;

    auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("Somewhere_Mono_441k.wav");
    auto outputFile = RelativeFilePath::getOutputFileFromProjectRoot("Somewhere_Mono_441k_Transposed.wav");
    bool success = fileProcessor.init(inputFile, outputFile);
    
    CHECK(success);
    CHECK(fileProcessor.getNumReadSamples() == 1531887);
}