#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+
#include "../SOURCE/Interpolator.h"


TEST_CASE("Can Interpolate linearly between two values")
{
    CHECK(Interpolator::linearInterp(0.0, 1.0, 0.2) == 0.2);
    CHECK(Interpolator::linearInterp(1.0, 0.0, 0.2) == 0.8);
    CHECK(Interpolator::linearInterp(-1.0, 0.0, 0.2) == -0.8);
    CHECK(Interpolator::linearInterp(0.0, -1.0, 0.2) == -0.2);
    CHECK(Interpolator::linearInterp(-0.2, -1.0, 0.5) == Catch::Approx(-0.6));


}