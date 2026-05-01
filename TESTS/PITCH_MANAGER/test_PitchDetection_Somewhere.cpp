// #include <catch2/catch_test_macros.hpp>
// #include <catch2/matchers/catch_matchers_floating_point.hpp>

// #include "../SUBMODULES/RD/SOURCE/PITCH/PitchDetector.h"
// #include "../SUBMODULES/RD/SOURCE/AudioFileProcessor.h"

// TEST_CASE("PitchDetector can detect pitch in Somewhere_Mono.wav file", "[PitchDetector][FilePitchDetect]")
// {
//     // Load the Somewhere file
//     juce::File somewhereFile = juce::File::getCurrentWorkingDirectory()
//         .getChildFile("SUBMODULES")
//         .getChildFile("RD")
//         .getChildFile("TESTS")
//         .getChildFile("GOLDEN")
//         .getChildFile("GOLDEN_Somewhere_Mono_441K.wav");

//     INFO("Looking for file at: " << somewhereFile.getFullPathName().toStdString());
//     REQUIRE(somewhereFile.existsAsFile());

//     // Load audio file
//     juce::AudioFormatManager formatManager;
//     formatManager.registerBasicFormats();

//     std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(somewhereFile));
//     REQUIRE(reader != nullptr);

//     const double sampleRate = reader->sampleRate;
//     const int numSamples = static_cast<int>(reader->lengthInSamples);
//     const int numChannels = reader->numChannels;

//     INFO("File info - Sample rate: " << sampleRate << " Hz, Samples: " << numSamples << ", Channels: " << numChannels);

//     // Expected pitch periods for G notes at different sample rates
//     // G3 = 196 Hz, G4 = 392 Hz
//     float expectedPeriodG3 = static_cast<float>(sampleRate) / 196.0f;  // ~245 samples at 48kHz
//     float expectedPeriodG4 = static_cast<float>(sampleRate) / 392.0f;  // ~122 samples at 48kHz

//     INFO("Expected periods - G3: " << expectedPeriodG3 << " samples, G4: " << expectedPeriodG4 << " samples");

//     SECTION("Can detect pitch in first vocal phrase (after any silence)")
//     {
//         // Read a chunk starting after potential intro silence
//         // Try from 2.0 seconds in to find louder vocal section
//         const int startSample = static_cast<int>(sampleRate * 2.0);
//         const int bufferSize = 2048;  // Standard pitch detection buffer size

//         juce::AudioBuffer<float> audioBuffer(numChannels, bufferSize);
//         reader->read(&audioBuffer, 0, bufferSize, startSample, true, true);

//         // Check RMS to ensure we have audio (not silence)
//         float rms = audioBuffer.getRMSLevel(0, 0, bufferSize);
//         INFO("Buffer RMS (before amplification): " << rms);
//         REQUIRE(rms > 0.001f);  // Should have some signal

//         // Amplify the audio if it's too quiet (normalize to ~0.2 RMS)
//         if (rms < 0.1f && rms > 0.0f)
//         {
//             float gain = 0.2f / rms;
//             audioBuffer.applyGain(gain);
//             float newRMS = audioBuffer.getRMSLevel(0, 0, bufferSize);
//             INFO("Buffer RMS (after amplification): " << newRMS << ", gain applied: " << gain);
//         }

//         // Run pitch detection
//         PitchDetector pitchDetector;
//         pitchDetector.prepareToPlay(bufferSize);

//         float detectedPeriod = pitchDetector.process(audioBuffer);

//         INFO("Detected period: " << detectedPeriod << " samples");

//         // Should detect a valid pitch (not -1 failure)
//         REQUIRE(detectedPeriod > 0);

//         // Should be within reasonable range for vocal pitch (not maxed out at 511.5)
//         REQUIRE(detectedPeriod < 500);

//         // Should be somewhere in the range of G3-G4 (allow octave errors)
//         // G2 (98Hz) to G5 (784Hz) range
//         float minPeriod = static_cast<float>(sampleRate) / 784.0f;  // G5: ~61 samples at 48kHz
//         float maxPeriod = static_cast<float>(sampleRate) / 98.0f;   // G2: ~490 samples at 48kHz

//         CHECK(detectedPeriod >= minPeriod);
//         CHECK(detectedPeriod <= maxPeriod);

//         // Calculate detected frequency
//         float detectedFreq = static_cast<float>(sampleRate) / detectedPeriod;
//         INFO("Detected frequency: " << detectedFreq << " Hz");

//         // Check if it's close to a G note (accounting for octave errors)
//         // G notes: G2=98, G3=196, G4=392, G5=784
//         bool isNearGNote = (std::abs(detectedFreq - 98.0f) < 20.0f) ||
//                           (std::abs(detectedFreq - 196.0f) < 20.0f) ||
//                           (std::abs(detectedFreq - 392.0f) < 40.0f) ||
//                           (std::abs(detectedFreq - 784.0f) < 80.0f);

//         CHECK(isNearGNote);
//     }

//     SECTION("Can detect pitch at multiple points in the file")
//     {
//         // Test at several time points to ensure consistent detection
//         std::vector<float> detectedPeriods;

//         for (int testPoint = 0; testPoint < 3; ++testPoint)
//         {
//             const int startSample = static_cast<int>(sampleRate * (0.5 + testPoint * 0.5));  // 0.5s, 1.0s, 1.5s
//             const int bufferSize = 2048;

//             if (startSample + bufferSize >= numSamples)
//                 break;

//             juce::AudioBuffer<float> audioBuffer(numChannels, bufferSize);
//             reader->read(&audioBuffer, 0, bufferSize, startSample, true, true);

//             float rms = audioBuffer.getRMSLevel(0, 0, bufferSize);
//             if (rms < 0.001f)
//                 continue;  // Skip silent sections

//             PitchDetector pitchDetector;
//             pitchDetector.prepareToPlay(bufferSize);

//             float detectedPeriod = pitchDetector.process(audioBuffer);

//             INFO("Test point " << testPoint << " - RMS: " << rms << ", detected period: " << detectedPeriod);

//             if (detectedPeriod > 0)
//             {
//                 detectedPeriods.push_back(detectedPeriod);

//                 // Should not be maxed out
//                 CHECK(detectedPeriod < 500);
//             }
//         }

//         // Should have detected pitch in at least one location
//         REQUIRE(detectedPeriods.size() > 0);
//     }
// }
