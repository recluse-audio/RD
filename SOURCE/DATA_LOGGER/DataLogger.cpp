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

juce::File DataLogger::createDataLogFile()
{
    auto timestamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    auto sessionDir = mOutputFile.getChildFile (timestamp);
    sessionDir.createDirectory();

    auto logFile = sessionDir.getChildFile ("output.txt");
    logFile.replaceWithText ("DataLogger Default Output");

    return logFile;
}
