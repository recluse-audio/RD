/**
 * Created By Ryan Devens on 2026-04-24 With Peace And Love
 */
#pragma once

#include <vector>
#include "../Util/Juce_Header.h"

/**
 * This class is meant to handle state and output of all sorts of data.
 * Primarily json, csv, xml.
 *
 * A DataLogger's "output" is always a directory containing files — never a
 * loose file. The output directory's location is built from two pieces:
 *   - mParentDirectory     : container folder that will hold this logger's
 *                            output directory.
 *   - mOutputDirectoryName : name (not full path) of this logger's output
 *                            directory inside mParentDirectory.
 * Together, getOutputDirectory() == mParentDirectory / mOutputDirectoryName.
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

    void setParentDirectory (const juce::File& parentDirectory);
    const juce::File& getParentDirectory() const;

    void setOutputDirectoryName (const juce::String& name);
    const juce::String& getOutputDirectoryName() const;

    /** Returns mParentDirectory / mOutputDirectoryName. Does not create on disk. */
    juce::File getOutputDirectory() const;

    /** Utility: creates an arbitrary directory on disk. */
    bool createOutputDirectory (const juce::File& directory);

    /** Creates this logger's own output directory (mParentDirectory / mOutputDirectoryName). */
    bool createOutputDirectory();

    // This function calls the overridden `createDataLogFile()` if logging enabled,
    // then cascades logData() to each registered child.
    bool logData();
    virtual juce::File createDataLogFile();

    // Non-owning. Caller keeps child alive for lifetime of registration.
    void addChild (DataLogger* child);
    void removeChild (DataLogger* child);
    size_t getNumChildren() const;

private:
    bool mIsLogging = false;
    juce::File   mParentDirectory     { juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("RD_DataLogger") };
    juce::String mOutputDirectoryName { "default" };
    std::vector<DataLogger*> mChildren;
};
