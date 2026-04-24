#include "DataLogger.h"

void DataLogger::setIsLogging (bool isLogging)
{
    mIsLogging = isLogging;
}

bool DataLogger::getIsLogging() const
{
    return mIsLogging;
}
