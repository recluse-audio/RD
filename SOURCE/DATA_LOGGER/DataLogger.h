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
 * loose file. The output directory's location is composed from:
 *   - parent directory : derived dynamically. If this logger has no parent
 *                        logger, it equals mDataLogRootDirectory. If it has
 *                        a parent logger, it equals the parent's
 *                        getDataLogOutputDirectory() so child output always
 *                        nests under the parent's current dir.
 *   - mDataLogOutputName : leaf folder name (defaults to construction
 *                          timestamp) inside the parent directory.
 * Together, getDataLogOutputDirectory() == getDataLogParentDirectory() / mDataLogOutputName.
 *
 * Users configure top-level loggers via setDataLogRootDirectory(). Child
 * loggers' parent directory is derived from their parent logger.
 *
 * THIS IS NOT MEANT TO BE REAL TIME SAFE
 */
class DataLogger
{
public:
    DataLogger();
    ~DataLogger() = default;

    void setIsLogging (bool isLogging);
    bool getIsLogging() const;

    void setDataLogRootDirectory (const juce::File& rootDirectory);
    const juce::File& getDataLogRootDirectory() const;

    void setDataLogOutputName (const juce::String& name);
    const juce::String& getDataLogOutputName() const;

    /** Read-only — derived dynamically from parent logger or root. */
    juce::File getDataLogParentDirectory() const;

    /** Returns getDataLogParentDirectory() / mDataLogOutputName. */
    juce::File getDataLogOutputDirectory() const;

    // Template method: short-circuits on !mIsLogging, creates output
    // directory if missing, calls the overridable doLogData() hook, then
    // cascades logData() to each registered child.
    bool logData();

    // Override hook for subclasses. Default writes the standard data log event
    // file via _createDataLogEventFile() and reports whether it exists.
    virtual bool doLogData();



    // Non-owning. Caller keeps child alive for lifetime of registration.
    // On addChild, the child stores a back-pointer to this logger as its
    // mParentLogger. The child's getDataLogParentDirectory() walks up
    // to mParentLogger->getDataLogOutputDirectory() dynamically:
    //   child.getDataLogOutputDirectory() == parent.getDataLogOutputDirectory() / child.getDataLogOutputName()
    void addChild (DataLogger* child);
    void removeChild (DataLogger* child);
    size_t getNumChildren() const;
    DataLogger* getChild (size_t index) const;

    DataLogger* getParentLogger() const;


private:
    bool mIsLogging = false;
    juce::File   mDataLogRootDirectory   { juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                              .getChildFile ("Recluse Audio")
                                              .getChildFile ("Data Logs") };

    juce::File _createDataLogEventFile();
    juce::String mCurrentTime;
    juce::String mDataLogOutputName;
    std::vector<DataLogger*> mChildren;
    DataLogger*  mParentLogger = nullptr;
};
