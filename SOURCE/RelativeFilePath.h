#pragma once
#include "Util/Juce_Header.h"

/**
 * @brief These functions allow easy construction of a juce::File from the cwd of one of my project repos.
 * 
 * Comes up a lot in tests, but may help elsewhere.  
 * 
 * Choosing to make this after copy/pasting the same boilerplate 3x
 * 
 */

class RelativeFilePath
{
public:

    // Allows specification of file from root dir of project
    static juce::File getFileFromProjectRoot(juce::StringRef relativePath)
    {
        juce::File currentDir = juce::File::getCurrentWorkingDirectory(); // this works when called from root dir of repo
        juce::String fullPath = currentDir.getFullPathName() + relativePath;
        // Instantiate the juce::File using the relative path
        juce::File file(fullPath);

        return file;
    } 

};