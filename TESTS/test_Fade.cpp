#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../SOURCE/FX/Fade.h"

TEST_CASE("Fade initial state is kNoFade", "[Fade]")
{
    Fade fade;
    REQUIRE(fade.getCurrentState() == Fade::FadeState::kNoFade);
}

TEST_CASE("Fading out transitions to kFullFade when value reaches zero", "[Fade]")
{
    Fade fade;
    fade.triggerFadeOut();

    fade.incrementFadeValue (Fade::kFadeLength);

    REQUIRE(fade.getCurrentState() == Fade::FadeState::kFullFade);
    REQUIRE(fade.getCurrentFadeValue() == Catch::Approx(0.0));
}

TEST_CASE("Fade setLength updates the fade increment", "[Fade]")
{
    Fade fade;

    SECTION("setLength of 2048 produces increment of 1/2048")
    {
        fade.setLength (2048);
        fade.triggerFadeOut();
        fade.incrementFadeValue (1);
        REQUIRE(fade.getCurrentFadeValue() == Catch::Approx(1.0 - (1.0 / 2048.0)));
    }

    SECTION("setLength of 8192 produces increment of 1/8192")
    {
        fade.setLength (8192);
        fade.triggerFadeOut();
        fade.incrementFadeValue (1);
        REQUIRE(fade.getCurrentFadeValue() == Catch::Approx(1.0 - (1.0 / 8192.0)));
    }
}
