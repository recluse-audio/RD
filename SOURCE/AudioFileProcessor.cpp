// #include "AudioFileProcessor.h"
// #include "AudioFileHelpers.h"
// #include "BufferFiller.h"
// #include "BufferWriter.h"

// //======================
// AudioFileProcessor::AudioFileProcessor()
// {
//     //reset();
// }

// //======================
// AudioFileProcessor::~AudioFileProcessor()
// {
//     reset();
// }



// //======================
// AudioFileProcessor::Result AudioFileProcessor::processFile(juce::File& inputFile, juce::File& outputFile,
//                                                              juce::AudioProcessor& processor, double sampleRate, int blockSize)
// {
//     BufferFiller::loadFromWavFile(inputFile, mReadBuffer);
//     BufferWriter::writeToWav(mReadBuffer, outputFile, sampleRate);
//     //BufferWriter::write

//     // updated from reader, used with writer
//     // int numChannels = 2;
//     // int numSamples = AudioFileHelpers::getFileLengthInSamples(inputFile);

//     // mProcessedBuffer.setSize(numChannels, numSamples);
//     // mProcessedBuffer.clear();

//     // juce::MidiBuffer midiBuffer;
//     // juce::AudioBuffer<float> processBlock(numChannels, blockSize);
//     // processBlock.clear();

//     // int blockIndex = 0;

//     // while(blockIndex < numSamples)
//     // {
//     //     AudioFileHelpers::readRange(inputFile, processBlock, blockIndex);

//     //     processor.processBlock(processBlock, midiBuffer);

//     //     for(int ch = 0; ch < numChannels; ch++)
//     //     {
//     //         for(int sampleIndex = 0; sampleIndex < blockSize; sampleIndex++)
//     //         {
//     //             int writeIndex = blockIndex + sampleIndex;
//     //             if(writeIndex >= numSamples)
//     //                 continue;

//     //             auto sample = processBlock.getSample(ch, sampleIndex);
//     //             mProcessedBuffer.setSample(ch, writeIndex, sample);
//     //         }
//     //     }

//     //     blockIndex += blockSize;
//     // }
//     // read input file first, this loads the internal mReadBuffer
//     // auto readResult = _readFileIntoInternalBuffer(inputFile, sampleRate, numChannels, numSamples);
//     // if(readResult != Result::kSuccess)
//     //     return AudioFileProcessor::Result::kFileReadError;

//     // auto readRMS = mReadBuffer.getRMSLevel(0, 0, mReadBuffer.getNumSamples());
//     // if(readRMS == 0.f)
//     //     return AudioFileProcessor::Result::kReadBufferError;

//     // auto processResult = _processInternalBufferWithProcessor(processor, blockSize);

//     // // write to output file
//     // auto writeResult = _writeInternalBufferToFile(outputFile, sampleRate, numChannels, numSamples);
//     // if(writeResult != AudioFileProcessor::Result::kSuccess)
//     //     return AudioFileProcessor::Result::kFileWriteError;

//     return AudioFileProcessor::Result::kSuccess;
    
// }

// //=========================
// AudioFileProcessor::Result AudioFileProcessor::_readFileIntoInternalBuffer(const juce::File& file, double& sampleRate, int& numChannels, int& numSamples)
// {


//     bool loadSuccess = BufferFiller::loadFromWavFile(file, mReadBuffer);
//     if(!loadSuccess)
//         return AudioFileProcessor::Result::kReadBufferError;
//     // // Initialize the AudioFormatManager
//     // juce::AudioFormatManager formatManager;
//     // formatManager.registerBasicFormats();

//     // // Create the reader for the input file
//     // mReader.reset(formatManager.createReaderFor(file));
//     // if (mReader == nullptr)
//     //     return AudioFileProcessor::Result::kFileReadError;

//     // // update input file data
//     // sampleRate = mReader->sampleRate;
//     // numChannels = mReader->numChannels;
//     // numSamples = mReader->lengthInSamples;

//     // // update internal buffer size
//     // mReadBuffer.clear(); 
//     // // always 2ch, either stereo or dual mono
//     // mReadBuffer.setSize(2, numSamples); 

//     // mReader->read(&mReadBuffer, 0, numSamples, 0, true, true);

//     return AudioFileProcessor::Result::kSuccess;

// }



// //=========================
// AudioFileProcessor::Result AudioFileProcessor::_writeInternalBufferToFile(const juce::File& file, double sampleRate, int numChannels, int numSamples)
// {
//     // Create output file stream
//     mOutputStream = std::make_unique<juce::FileOutputStream>(file);
//     if (mOutputStream == nullptr || !mOutputStream->openedOk())
//         return AudioFileProcessor::Result::kFileWriteError;

//     // Create the writer
//     juce::WavAudioFormat wavFormat;

//     mWriter.reset(wavFormat.createWriterFor(
//         mOutputStream.get(),               // Output stream
//         sampleRate,               // Sample rate
//         numChannels,              // Number of channels
//         24,                               // Bits per sample
//         {},                               // No metadata
//         0));   // No compression); 

//     mWriter->writeFromAudioSampleBuffer(mProcessedBuffer, 0, numSamples);

//     return AudioFileProcessor::Result::kSuccess;

// }



// //======================
// AudioFileProcessor::Result AudioFileProcessor::_processInternalBufferWithProcessor(juce::AudioProcessor& processor, int blockSize)
// {
//     int numChannels = mReadBuffer.getNumChannels();
//     int numSamples = mReadBuffer.getNumSamples();

//     // about to process internal buffer, so resize the buffer the processed audio is going to 
//     mProcessedBuffer.setSize(mReadBuffer.getNumChannels(), mReadBuffer.getNumSamples());
//     mProcessedBuffer.clear();

//     int blockStartIndex = 0;

//     juce::AudioBuffer<float> processBlock(numChannels, blockSize);
//     juce::MidiBuffer midiBuffer;

//     while(blockStartIndex < mProcessedBuffer.getNumSamples())
//     {
//         processBlock.clear();
//         // fill processBlock buffer from buffer we filled from read file
//         for(int ch = 0; ch < numChannels; ch++)
//         {
//             for(int sampleIndex = 0; sampleIndex < blockSize; sampleIndex++)
//             {
//                 int readIndex = blockStartIndex + sampleIndex;

//                 auto readSample = 0.f;

//                 // assuming readIndex is in range, we will assign value to readSample
//                 if(readIndex <= mReadBuffer.getNumSamples())
//                     readSample = mReadBuffer.getSample(ch, readIndex);

//                 processBlock.setSample(ch, sampleIndex, readSample);
//             }
//         }

//        // processor.processBlock(processBlock, midiBuffer);

//         for(int ch = 0; ch < numChannels; ch++)
//         {
//             for(int sampleIndex = 0; sampleIndex < blockSize; sampleIndex++)
//             {
//                 int writeIndex = blockStartIndex + sampleIndex;
//                 // don't write past end of mProcessedBuffer
//                 if(writeIndex >= mProcessedBuffer.getNumSamples())
//                     continue;

//                 auto writeSample = processBlock.getSample(ch, sampleIndex);

//                 mProcessedBuffer.setSample(ch, writeIndex, writeSample);
//             }
//         }

//         // increment block size, breaks us out of while loop if over mProcessedBuffer.numSamples
//         blockStartIndex += blockSize;
//     }

//     return AudioFileProcessor::Result::kSuccess;

// }


// //======================
// bool AudioFileProcessor::init(juce::File& inputFile, juce::File& outputFile)
// {
//     // Initialize the AudioFormatManager
//     juce::AudioFormatManager formatManager;
//     formatManager.registerBasicFormats();

//     // Create the reader for the input file
//     mReader.reset(formatManager.createReaderFor(inputFile));
//     if (mReader == nullptr)
//         return false;

//     // update input file data
//     mTotalSamples = mReader->lengthInSamples;

//     // update internal buffer size
//     mReadBuffer.clear();
//     mReadBuffer.setSize(2, mTotalSamples);

//     // Create output file stream
//     mOutputStream = std::make_unique<juce::FileOutputStream>(outputFile);
//     if (mOutputStream == nullptr || !mOutputStream->openedOk())
//         return false;

//     // Create the writer
//     juce::WavAudioFormat wavFormat;

//     mWriter.reset(wavFormat.createWriterFor(
//         mOutputStream.get(),               // Output stream
//         mReader->sampleRate,               // Sample rate
//         mReader->numChannels,              // Number of channels
//         24,                               // Bits per sample
//         {},                               // No metadata
//         0));   // No compression); 

//     return mWriter != nullptr;
// }

// //======================
// bool AudioFileProcessor::readFromFile(const juce::File& readFile)
// {
//     return true;
// }


// //======================
// void AudioFileProcessor::reset()
// {
//     mOutputStream.release(); // MUST release before writer is reset
//     mWriter.reset();
//     mReader.reset();
//     mTotalSamples = 0;
//     mSamplesRead = 0;
// }

// //======================
// bool AudioFileProcessor::read(juce::AudioBuffer<float>& readBuffer, int startInFile)
// {
//     // Read data into the output buffer
//     int numChannels = readBuffer.getNumChannels();
//     int numSamples = readBuffer.getNumSamples();

//     bool useRightChannel = numChannels >= 2 ? true : false;

//     mReader->read(&readBuffer, 0, numSamples, startInFile, true, useRightChannel);
//     return true;
// }

// //======================
// bool AudioFileProcessor::write(juce::AudioBuffer<float>& writeBuffer, int startPos)
// {
//     mWriter->writeFromAudioSampleBuffer(writeBuffer, startPos, writeBuffer.getNumSamples());
//     return true;
// }

// //=======================
// const int AudioFileProcessor::getNumReadSamples()
// {
//     return mTotalSamples;
// }