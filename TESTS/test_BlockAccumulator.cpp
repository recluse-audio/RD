/**
 * @file test_BlockAccumulator.cpp
 * @brief Tests for BlockAccumulator
 */

#include "../SOURCE/BlockAccumulator.h"
#include <catch2/catch_test_macros.hpp>

class BlockAccumulatorTest
{
public:
    BlockAccumulatorTest(BlockAccumulator& ba) : mAccumulator(ba) {}

    int getActiveIdx()       { return mAccumulator.mActiveIdx; }
    int getWritePos()        { return mAccumulator.mWritePos; }
    int getQueuedWritePos()  { return mAccumulator.mQueuedWritePos; }
    bool getBlockReady()     { return mAccumulator.mBlockReady; }

private:
    BlockAccumulator& mAccumulator;
};

TEST_CASE("BlockAccumulator - setSize", "[BlockAccumulator]")
{
    BlockAccumulator accumulator;

    SECTION("Reports correct channel count and target block size after setSize()")
    {
        accumulator.setSize(2, 2048);

        REQUIRE(accumulator.getNumChannels() == 2);
        REQUIRE(accumulator.getTargetBlockSize() == 2048);
    }

    SECTION("Internal buffers match the requested size")
    {
        accumulator.setSize(2, 2048);

        // Push a full block to get access to getReadyBlock() and confirm its dimensions.
        juce::AudioBuffer<float> fullBlock(2, 2048);
        fullBlock.clear();
        accumulator.push(fullBlock);

        REQUIRE(accumulator.blockReady());
        REQUIRE(accumulator.getReadyBlock().getNumChannels() == 2);
        REQUIRE(accumulator.getReadyBlock().getNumSamples() == 2048);
    }

    SECTION("setSize() can be called multiple times with different values")
    {
        accumulator.setSize(1, 512);
        REQUIRE(accumulator.getNumChannels() == 1);
        REQUIRE(accumulator.getTargetBlockSize() == 512);

        accumulator.setSize(2, 1024);
        REQUIRE(accumulator.getNumChannels() == 2);
        REQUIRE(accumulator.getTargetBlockSize() == 1024);
    }

    SECTION("blockReady() is false after setSize()")
    {
        accumulator.setSize(2, 2048);
        REQUIRE_FALSE(accumulator.blockReady());
    }
}

TEST_CASE("BlockAccumulator - reset", "[BlockAccumulator]")
{
    BlockAccumulator accumulator;
    BlockAccumulatorTest tester(accumulator);

    accumulator.setSize(2, 2048);

    SECTION("reset() clears blockReady flag after a full block was pushed")
    {
        juce::AudioBuffer<float> fullBlock(2, 2048);
        fullBlock.clear();
        accumulator.push(fullBlock);
        REQUIRE(accumulator.blockReady());

        accumulator.reset();
        REQUIRE_FALSE(accumulator.blockReady());
    }

    SECTION("reset() zeroes internal write positions and active index")
    {
        juce::AudioBuffer<float> partialBlock(2, 512);
        partialBlock.clear();
        accumulator.push(partialBlock);

        accumulator.reset();

        REQUIRE(tester.getWritePos() == 0);
        REQUIRE(tester.getQueuedWritePos() == 0);
        REQUIRE(tester.getActiveIdx() == 0);
    }

    SECTION("accumulator behaves correctly after reset()")
    {
        // Fill it once, then reset and confirm a fresh accumulation works.
        juce::AudioBuffer<float> fullBlock(2, 2048);
        fullBlock.clear();
        accumulator.push(fullBlock);
        REQUIRE(accumulator.blockReady());

        accumulator.reset();
        REQUIRE_FALSE(accumulator.blockReady());

        accumulator.push(fullBlock);
        REQUIRE(accumulator.blockReady());
    }
}

TEST_CASE("BlockAccumulator - accumulate 64-sample blocks into 2048", "[BlockAccumulator]")
{
    BlockAccumulator accumulator;
    accumulator.setSize(2, 2048);

    const int blockSize     = 64;
    const int targetSize    = 2048;
    const int numBlocks     = targetSize / blockSize; // 32 pushes to fill

    juce::AudioBuffer<float> incomingBlock(2, blockSize);
    incomingBlock.clear();
    for (int ch = 0; ch < incomingBlock.getNumChannels(); ++ch)
        juce::FloatVectorOperations::fill(incomingBlock.getWritePointer(ch), 1.0f, blockSize);

    SECTION("blockReady() is false until all 32 blocks are pushed")
    {
        for (int i = 0; i < numBlocks - 1; ++i)
        {
            accumulator.push(incomingBlock);
            REQUIRE_FALSE(accumulator.blockReady());
        }

        accumulator.push(incomingBlock);
        REQUIRE(accumulator.blockReady());
    }
}
