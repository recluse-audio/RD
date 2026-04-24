/**
 * Created by Ryan Devens 2026-04-23
 */

#pragma once
#include "Util/Juce_Header.h"

class RD_Processor;

/**
 * This class is for logging data as it is processed from an AudioProcessor.
 *
 * Output format mirrors the XML layout JUCE uses for APVTS save/restore
 * (juce::ValueTree -> XmlElement -> String).
 */
class ProcessorDataLogger
{
public:
    ProcessorDataLogger();
    ~ProcessorDataLogger();

    /** Returns an XML string describing the processor. Root tag: "PROCESSOR". */
    juce::String getProcessorDataAsXmlString (RD_Processor& processor);

    /** Returns the raw XmlElement. Caller owns the returned pointer. */
    std::unique_ptr<juce::XmlElement> getProcessorDataAsXml (RD_Processor& processor);

private:

};
