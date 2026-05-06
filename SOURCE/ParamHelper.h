#pragma once
#include "Util/Juce_Header.h"

namespace RD
{

class ParamHelper
{
public:
    struct ParamValues
    {
        float worldValue;
        float normalizedValue;
    };

    // Sets the parameter identified by paramID to the given non-normalized (world) value.
    // Out-of-range values are clipped to [range.start, range.end] before being applied.
    // Returns true if the parameter was found and set, false otherwise.
    static bool setParamWorldValue (juce::AudioProcessorValueTreeState& apvts,
                                    const juce::String& paramID,
                                    float worldValue)
    {
        auto* param = apvts.getParameter (paramID);
        if (param == nullptr)
            return false;

        const auto& range = param->getNormalisableRange();
        const float clamped = juce::jlimit (range.start, range.end, worldValue);
        const float normalized = range.convertTo0to1 (clamped);

        param->setValueNotifyingHost (normalized);
        return true;
    }

    // Returns the parameter's current world (non-normalized) and normalized [0, 1] values.
    // If the parameter is not found, returns {0.0f, 0.0f}.
    static ParamValues getParamValues (juce::AudioProcessorValueTreeState& apvts,
                                       const juce::String& paramID)
    {
        auto* param = apvts.getParameter (paramID);
        if (param == nullptr)
            return { 0.0f, 0.0f };

        const float normalized = param->getValue();
        const float worldValue = param->getNormalisableRange().convertFrom0to1 (normalized);
        return { worldValue, normalized };
    }

    struct ParamRangeValues
    {
        ParamValues defaultValue;
        ParamValues min;
        ParamValues max;
    };

    // Returns the parameter's default, min, and max as (world, normalized) pairs.
    // If the parameter is not found, returns all zeros.
    static ParamRangeValues getParamRangeValues (juce::AudioProcessorValueTreeState& apvts,
                                                 const juce::String& paramID)
    {
        auto* param = apvts.getParameter (paramID);
        if (param == nullptr)
            return {};

        const auto& range = param->getNormalisableRange();
        const float defaultNorm = param->getDefaultValue();
        const float defaultWorld = range.convertFrom0to1 (defaultNorm);

        return {
            { defaultWorld, defaultNorm },
            { range.start, 0.0f },
            { range.end,   1.0f }
        };
    }
};

} // namespace RD
