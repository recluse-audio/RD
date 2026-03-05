#include <catch2/catch_test_macros.hpp>
#include "../SOURCE/FX/Fade.h"

TEST_CASE("Fade initial state is kNoFade", "[Fade]")
{
    Fade fade;
    REQUIRE(fade.getCurrentState() == Fade::FadeState::kNoFade);
}
