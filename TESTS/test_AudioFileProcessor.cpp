#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+

#include "../SOURCE/AudioFileProcessor.h"

TEST_CASE("Can read/write .wav without processing.")
{
    AudioFileProcessor fileProcessor;

}