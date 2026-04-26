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

void DataLogger::setOutputFile (const juce::File& newOutputFile)
{
    mOutputFile = newOutputFile;
}

const juce::File& DataLogger::getOutputFile() const
{
    return mOutputFile;
}

bool DataLogger::logData()
{
    if(!mIsLogging)
        return false;

    juce::File file = createDataLogFile();
    bool ok = file.exists();

    // for (auto* child : mChildren)
    // {
    //     if (child != nullptr)
    //         ok = child->logData() && ok;
    // }

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

bool DataLogger::createOutputDirectory (const juce::File& file)
{
    if (file.exists() && file.isDirectory())
        return true;

    auto result = file.createDirectory();
    return result.wasOk() && file.isDirectory();
}

juce::File DataLogger::createDataLogFile()
{
    createOutputDirectory (mOutputFile);

    auto logFile = mOutputFile.getChildFile ("output.txt");
    logFile.replaceWithText ("DataLogger Default Output");

    return logFile;
}
