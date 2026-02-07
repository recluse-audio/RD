#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_approx.hpp>

#include "../SUBMODULES/RD/SOURCE/PITCH/PitchDetector.h"
#include "../SUBMODULES/RD/SOURCE/AudioFileProcessor.h"
#include "../SUBMODULES/RD/SOURCE/BufferFiller.h"
#include "../SUBMODULES/RD/SOURCE/BufferHelper.h"
#include "TEST_UTILS/TestUtils.h"

// Note frequencies and their expected periods at 48kHz
// First arpeggio: C2-E2-G2-C3 (each 4 seconds)
// Second arpeggio: Gb1-Bb1-Db2-Gb2 (each 4 seconds)
struct NoteInfo 
{
    const char* name;
    float frequency;  // Hz
    float expectedPeriod;  // samples at 48kHz
    float startTime;  // seconds
};

static const std::vector<NoteInfo> notes = {
    {"C2",  65.41f, 733.8f,  0.0f},
    {"E2",  82.41f, 582.4f,  4.0f},
    {"G2",  98.00f, 489.8f,  8.0f},
    {"C3", 130.81f, 366.9f, 12.0f},
    {"Gb1", 46.25f, 1037.8f, 16.0f},
    {"Bb1", 58.27f, 823.7f, 20.0f},
    {"Db2", 69.30f, 692.5f, 24.0f},
    {"Gb2", 92.50f, 518.9f, 28.0f}
};
//========================================
TEST_CASE("Golden file exists and has expected metadata", "[PitchDetector][FilePitchDetect]")
{
    // Load the test file with known pitches
    juce::File testFile = TestUtils::getGoldenDirectory().getChildFile("GOLDEN_CMajArp_GbMajArp.wav");

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
}

//=========================================
TEST_CASE("PitchDetector accurately detects known pitches in arpeggio test file", "[PitchDetector][FilePitchDetect]")
{
    // Load the test file with known pitches
    juce::File testFile = TestUtils::getGoldenDirectory().getChildFile("GOLDEN_CMajArp_GbMajArp.wav");

    // Load audio file
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    juce::AudioBuffer<float> fileBuffer(2, 768000);
    BufferFiller::loadFromWavFile(testFile, fileBuffer);

    juce::dsp::AudioBlock<float> oneNoteBlock = BufferHelper::getRangeAsBlock(fileBuffer, 0, 191999);
    juce::dsp::AudioBlock<float> detectionBlock = oneNoteBlock.getSubBlock(48000, PitchDetectorMagicNumbers::DefaultDetectionSize);

    // each note is 4 sec, re-use this for each note
    juce::AudioBuffer<float> detectionBuffer(2, PitchDetectorMagicNumbers::DefaultDetectionSize); detectionBuffer.clear();

    BufferHelper::cloneBlockToBuffer(detectionBuffer, detectionBlock);

    // Run pitch detection
    PitchDetector pitchDetector;
    pitchDetector.prepareToPlay(PitchDetectorMagicNumbers::DefaultDetectionSize);
    pitchDetector.setThreshold(0.001f);
    float detectedPeriod = pitchDetector.process(detectionBuffer);

    // Check if detected period matches either fundamental (733.8) or octave up (366.94)
    bool matchesFundamental = (detectedPeriod == Catch::Approx(733.8f).margin(2.0f));
    bool matchesOctave = (detectedPeriod == Catch::Approx(366.94f).margin(2.0f));
    bool matchesExpected = matchesFundamental || matchesOctave;

    INFO("Detected period: " << detectedPeriod << " samples");
    INFO("Expected: 733.8 (fundamental) or 366.94 (octave up)");
    CHECK(matchesExpected);

}
