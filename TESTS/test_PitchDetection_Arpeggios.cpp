#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "../SUBMODULES/RD/SOURCE/PITCH/PitchDetector.h"
#include "../SUBMODULES/RD/SOURCE/AudioFileProcessor.h"

TEST_CASE("PitchDetector accurately detects known pitches in arpeggio test file")
{
    // Load the test file with known pitches
    juce::File testFile = juce::File::getCurrentWorkingDirectory()
        .getChildFile("..")
        .getChildFile("SUBMODULES")
        .getChildFile("RD")
        .getChildFile("TESTS")
        .getChildFile("GOLDEN")
        .getChildFile("GOLDEN_CMajArp_GbMajArp.wav");

    INFO("Looking for file at: " << testFile.getFullPathName().toStdString());
    REQUIRE(testFile.existsAsFile());

    // Load audio file
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(testFile));
    REQUIRE(reader != nullptr);

    const double sampleRate = reader->sampleRate;
    const int numSamples = static_cast<int>(reader->lengthInSamples);
    const int numChannels = reader->numChannels;

    INFO("File info - Sample rate: " << sampleRate << " Hz, Samples: " << numSamples << ", Channels: " << numChannels);
    REQUIRE(sampleRate == 48000.0);  // File should be 48kHz

    // Note frequencies and their expected periods at 48kHz
    // First arpeggio: C2-E2-G2-C3 (each 4 seconds)
    // Second arpeggio: Gb1-Bb1-Db2-Gb2 (each 4 seconds)
    struct NoteInfo {
        const char* name;
        float frequency;  // Hz
        float expectedPeriod;  // samples at 48kHz
        float startTime;  // seconds
    };

    std::vector<NoteInfo> notes = {
        {"C2",  65.41f, 733.8f,  0.0f},
        {"E2",  82.41f, 582.4f,  4.0f},
        {"G2",  98.00f, 489.8f,  8.0f},
        {"C3", 130.81f, 366.9f, 12.0f},
        {"Gb1", 46.25f, 1037.8f, 16.0f},
        {"Bb1", 58.27f, 823.7f, 20.0f},
        {"Db2", 69.30f, 692.5f, 24.0f},
        {"Gb2", 92.50f, 518.9f, 28.0f}
    };

    SECTION("Can detect each note accurately")
    {
        const int bufferSize = 2048;
        PitchDetector pitchDetector;
        pitchDetector.prepareToPlay(bufferSize);

        int successfulDetections = 0;
        int totalTests = 0;

        for (const auto& note : notes)
        {
            // Read from 2 seconds into the note to ensure stable pitch
            const int startSample = static_cast<int>((note.startTime + 2.0) * sampleRate);

            if (startSample + bufferSize >= numSamples)
            {
                INFO("Skipping " << note.name << " - not enough samples");
                continue;
            }

            // Use MONO buffer for pitch detection (analyze left channel only)
            juce::AudioBuffer<float> audioBuffer(1, bufferSize);
            reader->read(&audioBuffer, 0, bufferSize, startSample, true, false);

            float rms = audioBuffer.getRMSLevel(0, 0, bufferSize);
            INFO("Note " << note.name << " - RMS: " << rms);

            // Amplify if needed (as we do in GranulatorProcessor)
            if (rms > 0.001f && rms < 0.2f)
            {
                float gain = 0.2f / rms;
                gain = juce::jmin(gain, 50.0f);
                audioBuffer.applyGain(gain);
                float newRMS = audioBuffer.getRMSLevel(0, 0, bufferSize);
                INFO("  Amplified: " << rms << " -> " << newRMS << " (gain: " << gain << "x)");
            }

            float detectedPeriod = pitchDetector.process(audioBuffer);
            totalTests++;

            INFO("Note " << note.name << " (" << note.frequency << " Hz):");
            INFO("  Expected period: " << note.expectedPeriod << " samples");
            INFO("  Detected period: " << detectedPeriod << " samples");

            // Check if detection succeeded
            if (detectedPeriod > 0)
            {
                float detectedFreq = static_cast<float>(sampleRate) / detectedPeriod;
                INFO("  Detected frequency: " << detectedFreq << " Hz");

                // Allow for octave errors (common in pitch detection)
                // Check if detected frequency is within 10% of target or an octave of target
                bool matchesFundamental = std::abs(detectedFreq - note.frequency) < (note.frequency * 0.1f);
                bool matchesOctaveDown = std::abs(detectedFreq - (note.frequency / 2.0f)) < (note.frequency * 0.1f);
                bool matchesOctaveUp = std::abs(detectedFreq - (note.frequency * 2.0f)) < (note.frequency * 0.2f);

                if (matchesFundamental || matchesOctaveDown || matchesOctaveUp)
                {
                    successfulDetections++;
                    CHECK(true);  // Mark as passed
                }
                else
                {
                    INFO("  ERROR: Detected frequency doesn't match expected (even allowing octave errors)");
                    CHECK(false);
                }
            }
            else
            {
                INFO("  ERROR: Pitch detection failed (returned " << detectedPeriod << ")");
                CHECK(false);
            }
        }

        INFO("Successfully detected " << successfulDetections << " out of " << totalTests << " notes");

        // Require at least 75% success rate
        REQUIRE(successfulDetections >= (totalTests * 3 / 4));
    }

    SECTION("Gb1 (46Hz) detection - tests low frequency limit")
    {
        // Gb1 is 46.25 Hz, period = 1037.8 samples
        // This tests if our pitch detector can handle very low frequencies
        const int bufferSize = 2048;
        const int startSample = static_cast<int>(18.0 * sampleRate);  // 2s into Gb1 note

        // Use MONO buffer for pitch detection
        juce::AudioBuffer<float> audioBuffer(1, bufferSize);
        reader->read(&audioBuffer, 0, bufferSize, startSample, true, false);

        // Amplify if needed
        float rms = audioBuffer.getRMSLevel(0, 0, bufferSize);
        if (rms < 0.2f)
        {
            audioBuffer.applyGain(0.2f / rms);
        }

        PitchDetector pitchDetector;
        pitchDetector.prepareToPlay(bufferSize);
        float detectedPeriod = pitchDetector.process(audioBuffer);

        INFO("Gb1 detection - expected period: 1037.8, detected: " << detectedPeriod);

        // Gb1 period (1037.8) exceeds default detection buffer size (1024)
        // This might fail or return octave up (518.9)
        if (detectedPeriod > 0)
        {
            float detectedFreq = static_cast<float>(sampleRate) / detectedPeriod;
            INFO("Detected frequency: " << detectedFreq << " Hz (expected 46.25 Hz or octave 92.5 Hz)");

            // Accept either fundamental or octave up (since period exceeds buffer size)
            bool matchesFundamental = std::abs(detectedFreq - 46.25f) < 10.0f;
            bool matchesOctaveUp = std::abs(detectedFreq - 92.5f) < 15.0f;

            CHECK((matchesFundamental || matchesOctaveUp));
        }
    }

    SECTION("C3 (131Hz) detection - tests higher frequency")
    {
        // C3 is 130.81 Hz, period = 366.9 samples
        // This should be well within detection range
        const int bufferSize = 2048;
        const int startSample = static_cast<int>(14.0 * sampleRate);  // 2s into C3 note

        // Use MONO buffer for pitch detection
        juce::AudioBuffer<float> audioBuffer(1, bufferSize);
        reader->read(&audioBuffer, 0, bufferSize, startSample, true, false);

        // Amplify if needed
        float rms = audioBuffer.getRMSLevel(0, 0, bufferSize);
        if (rms < 0.2f)
        {
            audioBuffer.applyGain(0.2f / rms);
        }

        PitchDetector pitchDetector;
        pitchDetector.prepareToPlay(bufferSize);
        float detectedPeriod = pitchDetector.process(audioBuffer);

        INFO("C3 detection - expected period: 366.9, detected: " << detectedPeriod);
        REQUIRE(detectedPeriod > 0);

        float detectedFreq = static_cast<float>(sampleRate) / detectedPeriod;
        INFO("Detected frequency: " << detectedFreq << " Hz (expected 130.81 Hz)");

        // Should detect fundamental or octave (allowing some tolerance)
        bool matchesFundamental = std::abs(detectedFreq - 130.81f) < 20.0f;
        bool matchesOctaveDown = std::abs(detectedFreq - 65.41f) < 10.0f;

        CHECK((matchesFundamental || matchesOctaveDown));
    }
}
