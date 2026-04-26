#pragma once
#include "../Util/Juce_Header.h"
#include <functional>
#include <vector>

/**
 * @brief An assortment of functions for filling buffers with amplitude values in various patterns
 */
class BufferFiller
{
public:
    static juce::String getGoldenFilePath(juce::String fileName);

    static void fillFromArray(juce::AudioBuffer<float>& buffer, const std::vector<float>& array);

    static void fillChannelWithValue(juce::AudioBuffer<float>& buffer, int ch, int val);

    static void fillWithAllOnes(juce::AudioBuffer<float>& bufferToFill);

    static void fillWithValue(juce::AudioBuffer<float>& bufferToFill, float value);

    static bool fillRangeWithValue(juce::AudioBuffer<float>& buffer, int startIndex, int endIndex, float value, int channel = -1);

    static void fillAlternatingZeroOne(juce::AudioBuffer<float>& bufferToFill);

    static void fillIncremental(juce::AudioBuffer<float>& bufferToFill);

    static void fillIncrementalLooping(juce::AudioBuffer<float>& bufferToFill, int period);

    static void generateHanning(juce::AudioBuffer<float>& bufferToFill);

    static void generateTukey(juce::AudioBuffer<float>& bufferToFill, float alpha = 0.5f);

    static void generateSine(juce::AudioBuffer<float>& bufferToFill);

    static void generateSineCycles(juce::AudioBuffer<float>& bufferToFill, int period);

    static double generateSineCycles(juce::AudioBuffer<float>& bufferToFill, double period, double normalizedStartPhase);

    static void generateSineWithPhase(juce::AudioBuffer<float>& bufferToFill, float period, double startPhase);

    static void generateStereoSineWithPhaseVariance(juce::AudioBuffer<float>& bufferToFill);

    static bool loadFromWavFile(const juce::File& wavFile, juce::AudioBuffer<float>& buffer);

    static bool fillFromWavFile(const juce::File& wavFile,
                                juce::AudioBuffer<float>& destBuffer,
                                std::function<void(float)> progressCallback = nullptr);

    static bool loadFromJsonFile(const juce::File& jsonFile, juce::AudioBuffer<float>& buffer, const juce::String& key = "Channel_0");

    static bool loadFromCSV(juce::AudioBuffer<float>& buffer, const juce::String& csvPath);

    static bool fillWithJuceArray(juce::AudioBuffer<float>& buffer, const juce::Array<juce::Array<float>>& array);
};
