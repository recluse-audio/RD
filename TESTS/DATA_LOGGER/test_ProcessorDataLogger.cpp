#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../SOURCE/DATA_LOGGER/ProcessorDataLogger.h"
#include "../../SOURCE/PROCESSORS/GAIN/GainProcessor.h"
#include "../../SOURCE/PROCESSORS/GRAIN/GrainShifterProcessor.h"
#include "../TEST_UTILS/TestUtils.h"

namespace
{
    const juce::XmlElement* findParamById (const juce::XmlElement& paramsXml, const juce::String& id)
    {
        for (auto* child : paramsXml.getChildIterator())
            if (child->getStringAttribute ("id") == id)
                return child;
        return nullptr;
    }
}

TEST_CASE("ProcessorDataLogger xml contains processor name", "[ProcessorDataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    GainProcessor       processor;
    ProcessorDataLogger logger;

    const juce::String xmlString = logger.getProcessorDataAsXmlString (processor);

    const auto parsed = juce::XmlDocument::parse (xmlString);
    REQUIRE (parsed != nullptr);

    REQUIRE (parsed->hasTagName ("PROCESSOR"));
    REQUIRE (parsed->getStringAttribute ("name") == processor.getName());
}

TEST_CASE("ProcessorDataLogger xml contains APVTS with default gain for GainProcessor", "[ProcessorDataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    GainProcessor       processor;
    ProcessorDataLogger logger;

    const auto xml = logger.getProcessorDataAsXml (processor);
    REQUIRE (xml != nullptr);

    auto* paramsXml = xml->getChildByName ("Parameters");
    REQUIRE (paramsXml != nullptr);

    auto* gainParam = findParamById (*paramsXml, "gain");
    REQUIRE (gainParam != nullptr);
    REQUIRE_THAT (gainParam->getDoubleAttribute ("value"),
                  Catch::Matchers::WithinAbs (0.01, 1e-6));
}

TEST_CASE("ProcessorDataLogger xml contains APVTS with defaults for GrainShifterProcessor", "[ProcessorDataLogger]")
{
    TestUtils::SetupAndTeardown setup;

    GrainShifterProcessor processor;
    ProcessorDataLogger   logger;

    const auto xml = logger.getProcessorDataAsXml (processor);
    REQUIRE (xml != nullptr);

    auto* paramsXml = xml->getChildByName ("Parameters");
    REQUIRE (paramsXml != nullptr);

    auto* shiftRatio = findParamById (*paramsXml, "shift_ratio");
    REQUIRE (shiftRatio != nullptr);
    REQUIRE_THAT (shiftRatio->getDoubleAttribute ("value"),
                  Catch::Matchers::WithinAbs (1.0, 1e-6));

    auto* threshold = findParamById (*paramsXml, "pitch_threshold");
    REQUIRE (threshold != nullptr);

    auto* windowSize = findParamById (*paramsXml, "pitch_window_size");
    REQUIRE (windowSize != nullptr);
    REQUIRE (windowSize->getIntAttribute ("value") == 2); // "2048" default index

    auto* hopSize = findParamById (*paramsXml, "pitch_hop_size");
    REQUIRE (hopSize != nullptr);
    REQUIRE (hopSize->getIntAttribute ("value") == 3); // "2048" default index
}
