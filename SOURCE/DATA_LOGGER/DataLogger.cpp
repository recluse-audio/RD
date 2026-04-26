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
}

void DataLogger::removeChild (DataLogger* child)
{
    mChildren.erase (std::remove (mChildren.begin(), mChildren.end(), child), mChildren.end());
}

size_t DataLogger::getNumChildren() const
{
    return mChildren.size();
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
