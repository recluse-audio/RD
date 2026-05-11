#include "BufferFiller.h"
#include "RD_BUFFER/RD_Buffer.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

juce::String BufferFiller::getGoldenFilePath(juce::String fileName)
{
    juce::File currentDir = juce::File::getCurrentWorkingDirectory();
    juce::String relativePath = "/SUBMODULES//RD/TESTS/GOLDEN/";

    return currentDir.getFullPathName() + relativePath + fileName;
}

void BufferFiller::convert(const juce::AudioBuffer<float>& src, rd_dsp::RD_Buffer& dst)
{
    const int numChannels = src.getNumChannels();
    const int numSamples  = src.getNumSamples();

    dst.setSize(numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        std::memcpy(dst.getWritePointer(ch),
                    src.getReadPointer(ch),
                    static_cast<std::size_t>(numSamples) * sizeof(float));
    }
}

void BufferFiller::convert(const rd_dsp::RD_Buffer& src, juce::AudioBuffer<float>& dst)
{
    const int numChannels = src.getNumChannels();
    const int numSamples  = src.getNumSamples();

    dst.setSize(numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        std::memcpy(dst.getWritePointer(ch),
                    src.getReadPointer(ch),
                    static_cast<std::size_t>(numSamples) * sizeof(float));
    }
}

void BufferFiller::fillFromArray(juce::AudioBuffer<float>& buffer, const std::vector<float>& array)
{
    buffer.setSize(1, static_cast<int>(array.size()));

    for (int i = 0; i < array.size(); ++i)
    {
        buffer.setSample(0, i, array[i]);
    }
}

void BufferFiller::fillChannelWithValue(juce::AudioBuffer<float>& buffer, int ch, int val)
{
    if(ch >= buffer.getNumChannels())
        return;

    for(int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); sampleIndex++)
    {
        buffer.setSample(ch, sampleIndex, static_cast<float>(val));
    }
}

void BufferFiller::fillWithAllOnes(juce::AudioBuffer<float>& bufferToFill)
{
    bufferToFill.clear();
    auto writePtr = bufferToFill.getArrayOfWritePointers();

    for(int sampleIndex = 0; sampleIndex < bufferToFill.getNumSamples(); sampleIndex++)
    {
        for(int ch = 0; ch < bufferToFill.getNumChannels(); ch++)
        {
            writePtr[ch][sampleIndex] = 1.f;
        }
    }
}

void BufferFiller::fillWithValue(juce::AudioBuffer<float>& bufferToFill, float value)
{
    bufferToFill.clear();
    auto writePtr = bufferToFill.getArrayOfWritePointers();

    for(int sampleIndex = 0; sampleIndex < bufferToFill.getNumSamples(); sampleIndex++)
    {
        for(int ch = 0; ch < bufferToFill.getNumChannels(); ch++)
        {
            writePtr[ch][sampleIndex] = value;
        }
    }
}

bool BufferFiller::fillRangeWithValue(juce::AudioBuffer<float>& buffer, int startIndex, int endIndex, float value, int channel)
{
    if(startIndex < 0 || endIndex >= buffer.getNumSamples() || startIndex > endIndex)
        return false;

    if(channel != -1 && (channel < 0 || channel >= buffer.getNumChannels()))
        return false;

    auto writePtr = buffer.getArrayOfWritePointers();

    for(int sampleIndex = startIndex; sampleIndex <= endIndex; sampleIndex++)
    {
        if(channel == -1)
        {
            for(int ch = 0; ch < buffer.getNumChannels(); ch++)
            {
                writePtr[ch][sampleIndex] = value;
            }
        }
        else
        {
            writePtr[channel][sampleIndex] = value;
        }
    }

    return true;
}

void BufferFiller::fillAlternatingZeroOne(juce::AudioBuffer<float>& bufferToFill)
{
    bufferToFill.clear();
    auto writePtr = bufferToFill.getArrayOfWritePointers();

    bool shouldWriteZero = true;

    for(int sampleIndex = 0; sampleIndex < bufferToFill.getNumSamples(); sampleIndex++)
    {
        float sampleValue = 1.f;
        if(shouldWriteZero)
            sampleValue = 0.f;

        for(int ch = 0; ch < bufferToFill.getNumChannels(); ch++)
        {
            writePtr[ch][sampleIndex] = sampleValue;
        }

        shouldWriteZero = !shouldWriteZero;
    }
}

void BufferFiller::fillIncremental(juce::AudioBuffer<float>& bufferToFill)
{
    bufferToFill.clear();
    auto writePtr = bufferToFill.getArrayOfWritePointers();

    for(int sampleIndex = 0; sampleIndex < bufferToFill.getNumSamples(); sampleIndex++)
    {
        for(int ch = 0; ch < bufferToFill.getNumChannels(); ch++)
        {
            writePtr[ch][sampleIndex] = (float)sampleIndex;
        }
    }
}

void BufferFiller::fillIncrementalLooping(juce::AudioBuffer<float>& bufferToFill, int period)
{
    bufferToFill.clear();
    auto writePtr = bufferToFill.getArrayOfWritePointers();

    int sampleInPeriod = 0;

    for(int sampleIndex = 0; sampleIndex < bufferToFill.getNumSamples(); sampleIndex++)
    {
        for(int ch = 0; ch < bufferToFill.getNumChannels(); ch++)
        {
            writePtr[ch][sampleIndex] = (float)sampleInPeriod;
        }

        sampleInPeriod++;
        if(sampleInPeriod >= period)
            sampleInPeriod = sampleInPeriod - period;
    }
}

void BufferFiller::generateHanning(juce::AudioBuffer<float>& bufferToFill)
{
    bufferToFill.clear();

    auto writePtr = bufferToFill.getArrayOfWritePointers();
    int numSamples = bufferToFill.getNumSamples() - 1;

    for(int sampleIndex = 0; sampleIndex <= numSamples; sampleIndex++)
    {
        writePtr[0][sampleIndex] = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * sampleIndex / (numSamples)));
    }
}

void BufferFiller::generateTukey(juce::AudioBuffer<float>& bufferToFill, float alpha)
{
    bufferToFill.clear();

    alpha = juce::jlimit(0.0f, 1.0f, alpha);

    auto writePtr = bufferToFill.getArrayOfWritePointers();
    int numSamples = bufferToFill.getNumSamples();

    if (numSamples == 0)
        return;

    int taperSamples = static_cast<int>(alpha * (numSamples - 1) / 2.0f);

    for (int ch = 0; ch < bufferToFill.getNumChannels(); ch++)
    {
        for (int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
        {
            float value = 1.0f;

            if (sampleIndex < taperSamples)
            {
                value = 0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * sampleIndex / taperSamples));
            }
            else if (sampleIndex > numSamples - taperSamples - 1)
            {
                value = 0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * (numSamples - sampleIndex - 1) / taperSamples));
            }

            writePtr[ch][sampleIndex] = value;
        }
    }
}

void BufferFiller::generateSine(juce::AudioBuffer<float>& bufferToFill)
{
    bufferToFill.clear();
    auto numChannels = bufferToFill.getNumChannels();
    int numSamples = bufferToFill.getNumSamples();

    auto writeBuff = bufferToFill.getArrayOfWritePointers();
    for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
    {
        auto sample = std::sin( (sampleIndex * juce::MathConstants<float>::twoPi) / numSamples );
        for(int channel = 0; channel < numChannels; channel++)
        {
            writeBuff[channel][sampleIndex] = sample;
        }
    }
}

void BufferFiller::generateSineCycles(juce::AudioBuffer<float>& bufferToFill, int period)
{
    bufferToFill.clear();
    auto numChannels = bufferToFill.getNumChannels();
    int numSamples = bufferToFill.getNumSamples();

    auto writeBuff = bufferToFill.getArrayOfWritePointers();

    int writePos = 0;
    for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
    {
        float sample = std::sin( ((float)writePos * juce::MathConstants<float>::twoPi) / (float)period );

        writePos++;
        if(writePos >= period)
            writePos = writePos - period;

        for(int channel = 0; channel < numChannels; channel++)
        {
            writeBuff[channel][sampleIndex] = sample;
        }
    }
}

double BufferFiller::generateSineCycles(juce::AudioBuffer<float>& bufferToFill, double period, double normalizedStartPhase)
{
    bufferToFill.clear();
    auto numChannels = bufferToFill.getNumChannels();
    int numSamples = bufferToFill.getNumSamples();

    auto writeBuff = bufferToFill.getArrayOfWritePointers();

    double writePos = normalizedStartPhase * period;

    for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
    {
        double radianPhase = (writePos / period) * juce::MathConstants<double>::twoPi;
        double sample = std::sin( radianPhase );

        writePos++;
        if(writePos >= period)
            writePos = writePos - period;

        for(int channel = 0; channel < numChannels; channel++)
        {
            writeBuff[channel][sampleIndex] = sample;
        }
    }

    return writePos / period;
}

void BufferFiller::generateSineWithPhase(juce::AudioBuffer<float>& bufferToFill, float period, double startPhase)
{
    bufferToFill.clear();
    auto numChannels = bufferToFill.getNumChannels();
    int numSamples = bufferToFill.getNumSamples();

    auto writeBuff = bufferToFill.getArrayOfWritePointers();

    for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
    {
        double phase = startPhase + (static_cast<double>(sampleIndex) / period) * 2.0 * M_PI;
        float sample = static_cast<float>(std::sin(phase));

        for(int channel = 0; channel < numChannels; channel++)
        {
            writeBuff[channel][sampleIndex] = sample;
        }
    }
}

void BufferFiller::generateStereoSineWithPhaseVariance(juce::AudioBuffer<float>& bufferToFill)
{
    bufferToFill.clear();
    auto numChannels = bufferToFill.getNumChannels();
    int numSamples = bufferToFill.getNumSamples();

    if(numChannels != 2)
        return;

    auto writeBuff = bufferToFill.getArrayOfWritePointers();

    for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
    {
        auto leftSample = std::sin( (sampleIndex * juce::MathConstants<float>::twoPi) / numSamples );
        auto rightSample = std::sin( (sampleIndex * juce::MathConstants<float>::twoPi * 2.f ) / numSamples );

        writeBuff[0][sampleIndex] = leftSample;
        writeBuff[1][sampleIndex] = rightSample;
    }
}

bool BufferFiller::loadFromWavFile(const juce::File& wavFile, juce::AudioBuffer<float>& buffer)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(wavFile));
    if (reader.get() == nullptr)
    {
        DBG("Failed to create reader for WAV file.");
        return false;
    }

    buffer.setSize(static_cast<int>(reader->numChannels), static_cast<int>(reader->lengthInSamples));
    reader->read(&buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
    DBG("Successfully loaded buffer from WAV file.");
    return true;
}

bool BufferFiller::fillFromWavFile(const juce::File& wavFile,
                                   juce::AudioBuffer<float>& destBuffer,
                                   std::function<void(float)> progressCallback)
{
    if (destBuffer.getNumSamples() <= 0 || destBuffer.getNumChannels() <= 0)
    {
        DBG("Destination buffer has zero size.");
        return false;
    }

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(wavFile));
    if (reader.get() == nullptr)
    {
        DBG("Failed to create reader for WAV file.");
        return false;
    }

    const int fileSamples = static_cast<int>(reader->lengthInSamples);
    const int destSamples = destBuffer.getNumSamples();
    const int totalToRead = juce::jmin(fileSamples, destSamples);

    destBuffer.clear();

    const int chunkSize = 65536;
    int samplesDone = 0;
    while (samplesDone < totalToRead)
    {
        const int thisChunk = juce::jmin(chunkSize, totalToRead - samplesDone);
        if (! reader->read(&destBuffer, samplesDone, thisChunk, samplesDone, true, true))
        {
            DBG("Reader failed mid-read.");
            return false;
        }
        samplesDone += thisChunk;

        if (progressCallback)
            progressCallback(static_cast<float>(samplesDone) / static_cast<float>(totalToRead));
    }

    if (progressCallback)
        progressCallback(1.0f);

    DBG("Successfully loaded " << totalToRead << " samples from WAV file.");
    return true;
}

bool BufferFiller::loadFromJsonFile(const juce::File& jsonFile, juce::AudioBuffer<float>& buffer, const juce::String& key)
{
    juce::FileInputStream inputStream(jsonFile);
    if (!inputStream.openedOk())
    {
        DBG("Failed to open JSON file.");
        return false;
    }

    auto jsonParsed = juce::JSON::parse(inputStream);
    if (jsonParsed.isVoid() || !jsonParsed.isObject())
    {
        DBG("Failed to parse JSON file or JSON is not an array.");
        return false;
    }

    auto keyID = juce::Identifier(key);
    juce::var jsonObject = jsonParsed;
    if (!jsonObject.hasProperty(keyID))
    {
        DBG("Key not found in JSON file: " + key);
        return false;
    }

    auto jsonValue = jsonObject[keyID];
    if (!jsonValue.isArray())
    {
        DBG("Value associated with the key is not an array.");
        return false;
    }

    juce::Array<juce::var> jsonArray = *jsonValue.getArray();
    int numSamples = jsonArray.size();
    buffer.setSize(1, numSamples);
    buffer.clear();

    for (int i = 0; i < jsonArray.size(); ++i)
    {
        if (jsonArray[i].isDouble())
        {
            double sample = jsonArray[i];
            for (int ch = 0; ch < buffer.getNumChannels(); ch++)
                buffer.setSample(ch, i, (float)sample);
        }
        else
        {
            DBG("Invalid amplitude value in JSON file at index " + juce::String(i));
            return false;
        }
    }

    DBG("Successfully loaded buffer from JSON file.");
    return true;
}

bool BufferFiller::loadFromCSV(juce::AudioBuffer<float>& buffer, const juce::String& csvPath)
{
    if(!csvPath.endsWith(".csv"))
    {
        DBG("BufferFiller::loadFromCSV() - ERROR ->> Not a csv path. <<--");
        return false;
    }

    auto csvFile = std::make_unique<juce::File>(csvPath);
    if(!csvFile->exists())
    {
        DBG("BufferFiller::loadFromCSV() - ERROR ->>File at " + csvPath + " does not exist.<<-");
        return false;
    }

    juce::FileInputStream inputStream(*csvFile.get());
    if(!inputStream.openedOk())
    {
        DBG("Failed to create juce::FileInputStream.");
        return false;
    }

    if(inputStream.getTotalLength() <= 0)
    {
        DBG("FileInputStream is of zero length.");
        return false;
    }

    juce::StringArray lines;
    while (!inputStream.isExhausted())
        lines.add(inputStream.readNextLine());

    if (lines.size() < 2)
        return false;

    int numSamples = lines.size() - 1;
    juce::StringArray firstRow = juce::StringArray::fromTokens(lines[1], ",", "");
    int numChannels = firstRow.size() - 1;

    buffer.setSize(numChannels, numSamples);

    for (int i = 1; i < lines.size(); ++i)
    {
        juce::StringArray tokens = juce::StringArray::fromTokens(lines[i], ",", "");
        if (tokens.size() != numChannels + 1)
            continue;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.setSample(ch, i - 1, tokens[ch + 1].getFloatValue());
        }
    }

    return true;
}

bool BufferFiller::fillWithJuceArray(juce::AudioBuffer<float>& buffer, const juce::Array<juce::Array<float>>& array)
{
    int numChannels = array.size();
    int numSamples = array.getReference(0).size();

    buffer.setSize(numChannels, numSamples);
    buffer.clear();
    for(int ch = 0; ch < numChannels; ch++)
    {
        auto channelArray = array.getReference(ch);
        for(int sampleIndex = 0; sampleIndex < numSamples; sampleIndex++)
        {
            auto arrayValue = channelArray.getReference(sampleIndex);
            buffer.setSample(ch, sampleIndex, arrayValue);
        }
    }
    return true;
}
