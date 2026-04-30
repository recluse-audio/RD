#include "DataLogger.h"
#include <algorithm>

DataLogger::DataLogger()
{
    mCurrentTime = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    // By default the current time
    mDataLogOutputName = mCurrentTime;
}

void DataLogger::setIsLogging (bool isLogging)
{
    mIsLogging = isLogging;
}

bool DataLogger::getIsLogging() const
{
    return mIsLogging;
}

void DataLogger::setDataLogRootDirectory (const juce::File& rootDirectory)
{
    mDataLogRootDirectory = rootDirectory;
}

const juce::File& DataLogger::getDataLogRootDirectory() const
{
    return mDataLogRootDirectory;
}

void DataLogger::setDataLogOutputName (const juce::String& name)
{
    mDataLogOutputName = name;
}

const juce::String& DataLogger::getDataLogOutputName() const
{
    return mDataLogOutputName;
}

juce::File DataLogger::getDataLogParentDirectory() const
{
    // Parent directory derived dynamically:
    //   - has parent logger -> nest under parent's current output dir
    //   - no parent logger  -> sit directly under root dir
    if (mParentLogger != nullptr)
        return mParentLogger->getDataLogOutputDirectory();
    return mDataLogRootDirectory;
}

juce::File DataLogger::getDataLogOutputDirectory() const
{
    return getDataLogParentDirectory().getChildFile (mDataLogOutputName);
}

bool DataLogger::logData()
{
    if(!mIsLogging)
        return false;

    // Single point of output-directory creation. Idempotent: short-circuits
    // if the directory already exists.
    auto outputDir = getDataLogOutputDirectory();
    if (! (outputDir.exists() && outputDir.isDirectory()))
        outputDir.createDirectory();

    juce::File file = _createDataLogEventFile();

    bool ok = doLogData();

    for (auto* child : mChildren)
    {
        if (child != nullptr)
            ok = child->logData() && ok;
    }

    return ok;
}

bool DataLogger::doLogData()
{

    return true;
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

juce::File DataLogger::_createDataLogEventFile()
{
    auto logFile = getDataLogOutputDirectory().getChildFile ("output.txt");
    logFile.replaceWithText ("DataLogger Default Output");

    return logFile;
}
