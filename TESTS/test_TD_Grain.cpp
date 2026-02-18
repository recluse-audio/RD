/**
 * test_TD_Grain.cpp
 * Tests for TD_Grain class
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../SOURCE/PROCESSORS/TDPSOLA/TD_Grain.h"
#include "../SOURCE/CircularBuffer.h"
#include "../SOURCE/Window.h"
#include "../SOURCE/PITCH/SynthMark.h"
#include "../SOURCE/BufferFiller.h"

TEST_CASE("TD_Grain - Instantiation", "[TD_Grain]")
{
    Window window;
    CircularBuffer sourceBuffer;

    SECTION("Constructor with SynthMark creates a valid grain")
    {
        // Pitch mark at 128, period of 128 samples, no shift
        SynthMark mark(128, 0, 255, 128, 128.0f);
        constexpr juce::int64 lookahead = 512;

        TD_Grain grain(mark, window, sourceBuffer, lookahead);

        REQUIRE(grain.isValid());
        REQUIRE_FALSE(grain.isFinished());
        REQUIRE(grain.getCurrentIndex() == 0);
    }

    SECTION("Write range is synth range offset by lookahead")
    {
        // synthRangeStart = 128 - 128 = 0, synthRangeEnd = 128 + 128 - 1 = 255
        SynthMark mark(128, 0, 255, 128, 128.0f);
        constexpr juce::int64 lookahead = 2048;

        TD_Grain grain(mark, window, sourceBuffer, lookahead);

        REQUIRE(grain.getWriteRangeStart() == mark.synthRangeStart + lookahead);
        REQUIRE(grain.getWriteRangeEnd()   == mark.synthRangeEnd   + lookahead);
    }

    SECTION("setGrain() updates write range and resets index")
    {
        SynthMark firstMark(128, 0, 255, 128, 128.0f);
        SynthMark newMark(512, 256, 767, 512, 256.0f);
        constexpr juce::int64 lookahead = 512;

        TD_Grain grain(firstMark, window, sourceBuffer, lookahead);
        grain.setGrain(newMark, lookahead);

        REQUIRE(grain.isValid());
        REQUIRE(grain.getCurrentIndex() == 0);
        REQUIRE(grain.getWriteRangeStart() == newMark.synthRangeStart + lookahead);
        REQUIRE(grain.getWriteRangeEnd()   == newMark.synthRangeEnd   + lookahead);
    }
}

TEST_CASE("TD_Grain - getOverlapWithBlock()", "[TD_Grain]")
{
    Window window;
    CircularBuffer sourceBuffer;

    // Grain: synthRangeStart=0, synthRangeEnd=255, lookahead=512
    //        → writeRangeStart=512, writeRangeEnd=767
    SynthMark mark(128, 0, 255, 128, 128.0f);
    constexpr juce::int64 lookahead = 512;
    TD_Grain grain(mark, window, sourceBuffer, lookahead);

    // writeRangeStart = 512, writeRangeEnd = 767

    SECTION("Block entirely before grain - no overlap")
    {
        auto overlap = grain.getOverlapWithBlock(0, 512);
        REQUIRE(overlap.isEmpty());
    }

    SECTION("Block ends exactly at writeRangeStart - no overlap")
    {
        auto overlap = grain.getOverlapWithBlock(256, 512);
        REQUIRE(overlap.isEmpty());
    }

    SECTION("Block entirely after grain - no overlap")
    {
        auto overlap = grain.getOverlapWithBlock(800, 1024);
        REQUIRE(overlap.isEmpty());
    }

    SECTION("Block starts exactly at writeRangeEnd+1 - no overlap")
    {
        auto overlap = grain.getOverlapWithBlock(768, 1024);
        REQUIRE(overlap.isEmpty());
    }

    SECTION("Block exactly matches write range - full overlap")
    {
        auto overlap = grain.getOverlapWithBlock(512, 768);
        REQUIRE_FALSE(overlap.isEmpty());
        REQUIRE(overlap.getStart() == 512);
        REQUIRE(overlap.getEnd()   == 768);
    }

    SECTION("Block contains entire grain - full overlap")
    {
        auto overlap = grain.getOverlapWithBlock(0, 1024);
        REQUIRE_FALSE(overlap.isEmpty());
        REQUIRE(overlap.getStart() == 512);
        REQUIRE(overlap.getEnd()   == 768);
    }

    SECTION("Block overlaps start of grain - partial overlap")
    {
        // Block [256, 640) overlaps grain [512, 768) → overlap [512, 640)
        auto overlap = grain.getOverlapWithBlock(256, 640);
        REQUIRE_FALSE(overlap.isEmpty());
        REQUIRE(overlap.getStart() == 512);
        REQUIRE(overlap.getEnd()   == 640);
    }

    SECTION("Block overlaps end of grain - partial overlap")
    {
        // Block [640, 1024) overlaps grain [512, 768) → overlap [640, 768)
        auto overlap = grain.getOverlapWithBlock(640, 1024);
        REQUIRE_FALSE(overlap.isEmpty());
        REQUIRE(overlap.getStart() == 640);
        REQUIRE(overlap.getEnd()   == 768);
    }

    SECTION("Single-sample block inside grain")
    {
        auto overlap = grain.getOverlapWithBlock(600, 601);
        REQUIRE_FALSE(overlap.isEmpty());
        REQUIRE(overlap.getStart() == 600);
        REQUIRE(overlap.getEnd()   == 601);
    }

    SECTION("Single-sample block at writeRangeStart")
    {
        auto overlap = grain.getOverlapWithBlock(512, 513);
        REQUIRE_FALSE(overlap.isEmpty());
        REQUIRE(overlap.getStart() == 512);
        REQUIRE(overlap.getEnd()   == 513);
    }

    SECTION("Single-sample block at writeRangeEnd")
    {
        auto overlap = grain.getOverlapWithBlock(767, 768);
        REQUIRE_FALSE(overlap.isEmpty());
        REQUIRE(overlap.getStart() == 767);
        REQUIRE(overlap.getEnd()   == 768);
    }
}

// Helper: build a CircularBuffer filled with ones (single channel)
static CircularBuffer makeOnesBuffer(int numSamples)
{
    CircularBuffer cb;
    cb.setSize(1, numSamples);
    cb.pushValue(numSamples, 1.0f, 0, true);
    return cb;
}

// Helper: build a SynthMark whose synth range is [0, grainSize-1] and pitch range starts at 0
// outputPeriod = grainSize / 2  →  synthRangeLength = grainSize
static SynthMark makeGrainMark(int grainSize)
{
    juce::int64 half = grainSize / 2;
    // SynthMark(pitchMark, pitchStart, pitchEnd, synthMark, outputPeriod)
    // synthRangeStart = synthMark - outputPeriod = 0
    // synthRangeEnd   = synthMark + outputPeriod - 1 = grainSize - 1
    return SynthMark(half, 0, grainSize - 1, half, static_cast<float>(half));
}

TEST_CASE("TD_Grain - process() with ones buffer shows only window coefficients", "[TD_Grain][Window]")
{
    // With CircularBuffer full of 1.0f, the output equals the window values directly.
    // grainSize = synthRangeLength, which is what TD_Grain uses as grainPeriod internally.
    constexpr int grainSize = 512;

    SynthMark mark       = makeGrainMark(grainSize);
    CircularBuffer source = makeOnesBuffer(grainSize * 2);

    juce::AudioBuffer<float> output(1, grainSize);

    // All sections use window sized to grainSize so phaseIncrement = 1.0 (1:1 lookup).
    // This lets us compare against a same-sized reference buffer from BufferFiller directly.

    SECTION("kNone (rectangular) - output is all ones")
    {
        Window window;
        window.setSize(grainSize);
        window.setShape(Window::Shape::kNone);
        window.setPeriod(grainSize);

        output.clear();
        TD_Grain grain(mark, window, source, 0);
        grain.process(output, 0, grainSize);

        for (int i = 0; i < grainSize; ++i)
            REQUIRE(output.getSample(0, i) == 1.0f);
    }

    SECTION("kHanning - output matches Hanning coefficients")
    {
        Window window;
        window.setSize(grainSize);
        window.setShape(Window::Shape::kHanning);
        window.setPeriod(grainSize);

        juce::AudioBuffer<float> ref(1, grainSize);
        BufferFiller::generateHanning(ref);

        output.clear();
        TD_Grain grain(mark, window, source, 0);
        grain.process(output, 0, grainSize);

        for (int i = 0; i < grainSize; ++i)
        {
            INFO("sample " << i);
            REQUIRE_THAT(output.getSample(0, i),  Catch::Matchers::WithinAbs(ref.getSample(0, i), 1e-5f));
        }
    }

    SECTION("kTukey - output matches Tukey coefficients")
    {
        Window window;
        window.setSize(grainSize);
        window.setShape(Window::Shape::kTukey);
        window.setPeriod(grainSize);

        juce::AudioBuffer<float> ref(1, grainSize);
        BufferFiller::generateTukey(ref);

        output.clear();
        TD_Grain grain(mark, window, source, 0);
        grain.process(output, 0, grainSize);

        for (int i = 0; i < grainSize; ++i)
        {
            INFO("sample " << i);
            REQUIRE_THAT(output.getSample(0, i), Catch::Matchers::WithinAbs(ref.getSample(0, i), 1e-5f));
        }
    }

    SECTION("kTukey - starts and ends near zero, flat top in the middle")
    {
        Window window;
        window.setSize(grainSize);
        window.setShape(Window::Shape::kTukey);
        window.setPeriod(grainSize);

        output.clear();
        TD_Grain grain(mark, window, source, 0);
        grain.process(output, 0, grainSize);

        // Edges should taper to zero (alpha=0.5 → 25% taper each side = 128 samples)
        REQUIRE(output.getSample(0, 0)            < 0.01f);
        REQUIRE(output.getSample(0, grainSize - 1) < 0.01f);

        // Flat top should be 1.0
        REQUIRE_THAT(output.getSample(0, grainSize / 2), Catch::Matchers::WithinAbs(1.0f, 1e-5f));
    }
}
