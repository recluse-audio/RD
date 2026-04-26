#include "DataLogger.h"
#include <algorithm>

void DataLogger::setIsLogging (bool isLogging)
{
    mIsLogging = isLogging;
}

bool DataLogger::getIsLogging() const
{
    return mIsLogging;
}

void DataLogger::setParentDirectory (const juce::File& parentDirectory)
{
    mParentDirectory = parentDirectory;
}

const juce::File& DataLogger::getParentDirectory() const
{
    return mParentDirectory;
}

void DataLogger::setOutputDirectoryName (const juce::String& name)
{
    mOutputDirectoryName = name;
}

const juce::String& DataLogger::getOutputDirectoryName() const
{
    return mOutputDirectoryName;
}

juce::File DataLogger::getOutputDirectory() const
{
    return mParentDirectory.getChildFile (mOutputDirectoryName);
}

bool DataLogger::logData()
{
    if(!mIsLogging)
        return false;

    // Sync this logger's parent directory from the parent logger (if any)
    // so the child's output always nests under the parent's current output
    // directory at log time.
    if (mParentLogger != nullptr)
        setParentDirectory (mParentLogger->getOutputDirectory());

    juce::File file = createDataLogFile();
    bool ok = file.exists();

    for (auto* child : mChildren)
    {
        if (child != nullptr)
            ok = child->logData() && ok;
    }

    return ok;
}

void DataLogger::addChild (DataLogger* child)
{
    if (child == nullptr || child == this)
        return;

    if (std::find (mChildren.begin(), mChildren.end(), child) != mChildren.end())
        return;

    mChildren.push_back (child);
    child->mParentLogger = this;
}

void DataLogger::removeChild (DataLogger* child)
{
    auto it = std::find (mChildren.begin(), mChildren.end(), child);
    if (it == mChildren.end())
        return;

    if (child != nullptr && child->mParentLogger == this)
        child->mParentLogger = nullptr;

    mChildren.erase (it);
}

size_t DataLogger::getNumChildren() const
{
    return mChildren.size();
}

DataLogger* DataLogger::getParentLogger() const
{
    return mParentLogger;
}

bool DataLogger::createOutputDirectory (const juce::File& directory)
{
    if (directory.exists() && directory.isDirectory())
        return true;

    auto result = directory.createDirectory();
    return result.wasOk() && directory.isDirectory();
}

bool DataLogger::createOutputDirectory()
{
    return createOutputDirectory (getOutputDirectory());
}

juce::File DataLogger::createDataLogFile()
{
    createOutputDirectory();

    auto logFile = getOutputDirectory().getChildFile ("output.txt");
    logFile.replaceWithText ("DataLogger Default Output");

    return logFile;
}
