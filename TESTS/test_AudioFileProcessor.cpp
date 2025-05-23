// #include <catch2/catch_test_macros.hpp>
// #include <catch2/matchers/catch_matchers_string.hpp>
// #include <catch2/catch_approx.hpp>  // For Approx in Catch2 v3+
// #include "TEST_UTILS/TestUtils.h"

// #include "../SOURCE/AudioFileProcessor.h"
// #include "../SOURCE/RelativeFilePath.h"
// #include "../SOURCE/BufferFiller.h"
// #include "../SOURCE/BufferHelper.h"
// #include "../SOURCE/AudioFileHelpers.h"
// #include "../SOURCE/EFFECTS/GainProcessor.h"


// static const int kGoldenNumSamples = 1531887;
// static const int kGoldenNumChannels = 1;
// static const int kGoldenBufferNumSamples = 512;
// static const int kFileStartIndex = 88200;

// //
// TEST_CASE("Can read .wav")
// {
//     AudioFileProcessor fileProcessor;

//     auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
//     auto outputFile = RelativeFilePath::getOutputFileFromProjectRoot("Somewhere_Mono_441k_Transposed.wav");
//     bool success = fileProcessor.init(inputFile, outputFile);
//     // AudioFileProcessor should setup reader/writer/stream and return true
//     CHECK(success);


//     // AudioFileProcessor should update internal mTotalSamples correctly
//     CHECK(fileProcessor.getNumReadSamples() == kGoldenNumSamples);

//     // buffer to read from inputFile and write to outputFile
//     juce::AudioBuffer<float> buffer(kGoldenNumChannels, kGoldenBufferNumSamples);
//     buffer.clear();

//     fileProcessor.read(buffer, kFileStartIndex);

//     // filling buffer from golden json
//     juce::AudioBuffer<float> goldenBuffer(kGoldenNumChannels, kGoldenBufferNumSamples);
//     auto goldenJsonFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k_Two_Second_Mark.json");
//     BufferFiller::loadFromJsonFile(goldenJsonFile, goldenBuffer);

//     // check reading a section of golden wav using read() method agains golden json of chunk of 512 samples in wav file
//     bool matchesGolden = BufferHelper::buffersAreIdentical(buffer, goldenBuffer);
//     CHECK(matchesGolden);


// }



// //
// TEST_CASE("Read/Write with no processing.")
// {
//     TestUtils::SetupAndTeardown setupAndTeardown;

//     {
//         AudioFileProcessor fileProcessor;
//         GainProcessor processor;
//         processor.prepareToPlay(44100, 512);
//         processor.setGain(1.f);
        
//         auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
//         auto outputFile = RelativeFilePath::getOutputFileFromProjectRoot("AudioFileProcessor_Basic_Read_Write_Test_Output.wav");
        
//         auto result = fileProcessor.processFile(inputFile, outputFile, processor, 44100, 512);
//         CHECK(result == AudioFileProcessor::Result::kSuccess);
    
//         juce::AudioBuffer<float> goldenBuffer;
//         juce::AudioBuffer<float> testBuffer;
    
//         goldenBuffer.clear(); testBuffer.clear();
    
//         BufferFiller::loadFromWavFile(inputFile, goldenBuffer);
//         BufferFiller::loadFromWavFile(outputFile, testBuffer);
    
//         CHECK(goldenBuffer.getRMSLevel(0, 0, goldenBuffer.getNumSamples()) > 0.0);
//         CHECK(testBuffer.getRMSLevel(0, 0, testBuffer.getNumSamples()) > 0.0);
    
//         bool areIdentical = BufferHelper::buffersAreIdentical(goldenBuffer, testBuffer);
//         CHECK(areIdentical);
//     }

//     juce::DeletedAtShutdown::deleteAll();
// }

// //
// TEST_CASE("Read/Write with gain processing.", "[Process Audio File]")
// {
//     // TestUtils::SetupAndTeardown setupAndTeardown;

//     // {
//     //     AudioFileProcessor fileProcessor;
//     //     GainProcessor processor;
//     //     processor.prepareToPlay(44100, 512);
//     //     processor.setGain(0.1f);
        
//     //     auto inputFile = RelativeFilePath::getGoldenFileFromProjectRoot("GOLDEN_Somewhere_Mono_441k.wav");
//     //     auto outputFile = RelativeFilePath::getOutputFileFromProjectRoot("AudioFileProcessor_Gain_Read_Write_Test_Output.wav");
//     //     CHECK(outputFile.exists());

//     //     auto result = fileProcessor.processFile(inputFile, outputFile, processor, 44100, 512);
//     //     CHECK(result == AudioFileProcessor::Result::kSuccess);
    
//     //     juce::AudioBuffer<float> goldenBuffer;
//     //     juce::AudioBuffer<float> testBuffer;
    
//     //     goldenBuffer.clear(); testBuffer.clear();
    
//     //     BufferFiller::loadFromWavFile(inputFile, goldenBuffer);
//     //     BufferFiller::loadFromWavFile(outputFile, testBuffer);
    
//     //     CHECK(goldenBuffer.getRMSLevel(0, 0, goldenBuffer.getNumSamples()) > 0.0);
//     //     CHECK(testBuffer.getRMSLevel(0, 0, testBuffer.getNumSamples()) > 0.0);
    
//     //     bool areIdentical = BufferHelper::buffersAreIdentical(goldenBuffer, testBuffer);
//     //     CHECK(!areIdentical);
//     // }

//     // juce::DeletedAtShutdown::deleteAll();
// }