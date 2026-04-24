#include "ProcessorDataLogger.h"
#include "PROCESSORS/BASE/RD_Processor.h"

ProcessorDataLogger::ProcessorDataLogger()
{
}

ProcessorDataLogger::~ProcessorDataLogger()
{
}

std::unique_ptr<juce::XmlElement> ProcessorDataLogger::getProcessorDataAsXml (RD_Processor& processor)
{
    auto xml = std::make_unique<juce::XmlElement> ("PROCESSOR");
    xml->setAttribute ("name", processor.getName());

    auto& apvts = processor.getAPVTS();
    if (auto apvtsXml = apvts.copyState().createXml())
        xml->addChildElement (apvtsXml.release());

    return xml;
}

juce::String ProcessorDataLogger::getProcessorDataAsXmlString (RD_Processor& processor)
{
    const auto xml = getProcessorDataAsXml (processor);
    return xml->toString();
}
