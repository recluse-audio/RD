/**
 * Created By Ryan Devens on 2026-04-24 With Peace And Love
 */
#pragma once

#include <vector>
#include "../Util/Juce_Header.h"

/**
 * This class is meant to handle state and output of all sorts of data.
 * Primarily json, csv, xml
 *
 * THIS IS NOT MEANT TO BE REAL TIME SAFE
 */
class DataLogger
{
public:
    DataLogger()  = default;
    ~DataLogger() = default;

    void setIsLogging (bool isLogging);
    bool getIsLogging() const;

    void setOutputFile (const juce::File& newOutputFile);
    const juce::File& getOutputFile() const;

    bool createOutputDirectory (const juce::File& file);

    // This function calls the overridden   createDataLogfile()` if the boolean allows
    bool logData();
    virtual juce::File createDataLogFile();

    // Non-owning. Caller keeps child alive for lifetime of registration.
    void addChild (DataLogger* child);
    void removeChild (DataLogger* child);
    size_t getNumChildren() const;

private:
    bool mIsLogging = false;
    juce::File mOutputFile { juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("RD_DataLogger") };
    std::vector<DataLogger*> mChildren;
};
