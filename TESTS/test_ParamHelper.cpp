#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../SOURCE/ParamHelper.h"
#include "../SOURCE/PROCESSORS/GAIN/GainProcessor.h"
#include "../SOURCE/PROCESSORS/GRAIN/GrainShifterProcessor.h"

// GainProcessor's "gain" param is float [0.0, 1.0] default 1.0  — see RD_Processor::_createParameterLayout.
// GrainShifterProcessor's "shift_ratio" is float [0.5, 2.0] default 1.0 — see GrainShifterProcessor::createParameterLayout.
// shift_ratio is the more useful one for clipping/normalization checks since its world range != [0,1].

TEST_CASE("setParamWorldValue sets a value in range")
{
    GrainShifterProcessor processor;
    REQUIRE (RD::ParamHelper::setParamWorldValue (processor.getAPVTS(), "shift_ratio", 1.5f));

    auto values = RD::ParamHelper::getParamValues (processor.getAPVTS(), "shift_ratio");
    CHECK (values.worldValue == Catch::Approx (1.5f).margin (0.01f));
    // shift_ratio range [0.5, 2.0] → 1.5 normalized = (1.5 - 0.5) / 1.5 ≈ 0.6667
    CHECK (values.normalizedValue == Catch::Approx (2.0f / 3.0f).margin (0.01f));
}

TEST_CASE("setParamWorldValue clips values above the range")
{
    GrainShifterProcessor processor;
    REQUIRE (RD::ParamHelper::setParamWorldValue (processor.getAPVTS(), "shift_ratio", 5.0f));

    auto values = RD::ParamHelper::getParamValues (processor.getAPVTS(), "shift_ratio");
    CHECK (values.worldValue == Catch::Approx (2.0f).margin (0.001f));
    CHECK (values.normalizedValue == Catch::Approx (1.0f).margin (0.001f));
}

TEST_CASE("setParamWorldValue clips values below the range")
{
    GrainShifterProcessor processor;
    REQUIRE (RD::ParamHelper::setParamWorldValue (processor.getAPVTS(), "shift_ratio", -10.0f));

    auto values = RD::ParamHelper::getParamValues (processor.getAPVTS(), "shift_ratio");
    CHECK (values.worldValue == Catch::Approx (0.5f).margin (0.001f));
    CHECK (values.normalizedValue == Catch::Approx (0.0f).margin (0.001f));
}

TEST_CASE("setParamWorldValue accepts the exact range endpoints")
{
    GrainShifterProcessor processor;

    REQUIRE (RD::ParamHelper::setParamWorldValue (processor.getAPVTS(), "shift_ratio", 0.5f));
    auto low = RD::ParamHelper::getParamValues (processor.getAPVTS(), "shift_ratio");
    CHECK (low.worldValue == Catch::Approx (0.5f).margin (0.001f));
    CHECK (low.normalizedValue == Catch::Approx (0.0f).margin (0.001f));

    REQUIRE (RD::ParamHelper::setParamWorldValue (processor.getAPVTS(), "shift_ratio", 2.0f));
    auto high = RD::ParamHelper::getParamValues (processor.getAPVTS(), "shift_ratio");
    CHECK (high.worldValue == Catch::Approx (2.0f).margin (0.001f));
    CHECK (high.normalizedValue == Catch::Approx (1.0f).margin (0.001f));
}

TEST_CASE("setParamWorldValue works on a different processor / param")
{
    GainProcessor processor;
    // GainProcessor's "gain" range is [0.0, 1.0] — world == normalized here.
    REQUIRE (RD::ParamHelper::setParamWorldValue (processor.getAPVTS(), "gain", 0.25f));

    auto values = RD::ParamHelper::getParamValues (processor.getAPVTS(), "gain");
    CHECK (values.worldValue == Catch::Approx (0.25f).margin (0.001f));
    CHECK (values.normalizedValue == Catch::Approx (0.25f).margin (0.001f));
}

TEST_CASE("getParamRangeValues returns default, min, and max for shift_ratio")
{
    GrainShifterProcessor processor;
    auto rv = RD::ParamHelper::getParamRangeValues (processor.getAPVTS(), "shift_ratio");

    // Default 1.0 in range [0.5, 2.0] → normalized = (1.0 - 0.5) / 1.5 ≈ 0.3333
    CHECK (rv.defaultValue.worldValue      == Catch::Approx (1.0f).margin (0.001f));
    CHECK (rv.defaultValue.normalizedValue == Catch::Approx (1.0f / 3.0f).margin (0.01f));

    CHECK (rv.min.worldValue      == Catch::Approx (0.5f).margin (0.001f));
    CHECK (rv.min.normalizedValue == Catch::Approx (0.0f).margin (0.001f));

    CHECK (rv.max.worldValue      == Catch::Approx (2.0f).margin (0.001f));
    CHECK (rv.max.normalizedValue == Catch::Approx (1.0f).margin (0.001f));
}

TEST_CASE("getParamRangeValues returns default, min, and max for gain")
{
    GainProcessor processor;
    auto rv = RD::ParamHelper::getParamRangeValues (processor.getAPVTS(), "gain");

    // gain range [0.0, 1.0], default 1.0 — world == normalized
    CHECK (rv.defaultValue.worldValue      == Catch::Approx (1.0f).margin (0.001f));
    CHECK (rv.defaultValue.normalizedValue == Catch::Approx (1.0f).margin (0.001f));

    CHECK (rv.min.worldValue      == Catch::Approx (0.0f).margin (0.001f));
    CHECK (rv.min.normalizedValue == Catch::Approx (0.0f).margin (0.001f));

    CHECK (rv.max.worldValue      == Catch::Approx (1.0f).margin (0.001f));
    CHECK (rv.max.normalizedValue == Catch::Approx (1.0f).margin (0.001f));
}

TEST_CASE("getParamRangeValues handles unknown paramID")
{
    GrainShifterProcessor processor;
    auto rv = RD::ParamHelper::getParamRangeValues (processor.getAPVTS(), "nonexistent");

    CHECK (rv.defaultValue.worldValue      == 0.0f);
    CHECK (rv.defaultValue.normalizedValue == 0.0f);
    CHECK (rv.min.worldValue      == 0.0f);
    CHECK (rv.min.normalizedValue == 0.0f);
    CHECK (rv.max.worldValue      == 0.0f);
    CHECK (rv.max.normalizedValue == 0.0f);
}

TEST_CASE("setParamWorldValue and getParamValues handle unknown paramID")
{
    GrainShifterProcessor processor;
    CHECK_FALSE (RD::ParamHelper::setParamWorldValue (processor.getAPVTS(), "nonexistent", 1.0f));

    auto values = RD::ParamHelper::getParamValues (processor.getAPVTS(), "nonexistent");
    CHECK (values.worldValue == 0.0f);
    CHECK (values.normalizedValue == 0.0f);
}
