/**
 * Created By Ryan Devens on 2026-04-24 With Peace And Love
 */
#pragma once

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

private:
    bool mIsLogging = false;
};
