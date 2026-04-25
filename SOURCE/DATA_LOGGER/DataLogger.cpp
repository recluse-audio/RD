#include "DataLogger.h"

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
    return file.exists();
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
