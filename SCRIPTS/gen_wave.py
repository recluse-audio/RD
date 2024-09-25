import unittest
import numpy as np
from scipy.io import wavfile  # Correct import for wavfile
import json

def gen_sin_wave(cycles, frequency, sample_rate=44100, amplitude=0.5):
    duration = cycles / frequency
    t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
    return amplitude * np.sin(2 * np.pi * frequency * t)

def gen_incremental(sample_rate):
    # Create an array with float values equal to their indices
    incremental_array = np.array([float(i) for i in range(sample_rate)])
    return incremental_array

# Saving as .wav and .json
def save_waveform(wave, sample_rate, filename_prefix):
    """
    Saves the waveform data to both .wav and .json files.

    :param wave: NumPy array containing the waveform data.
    :param sample_rate: Sample rate of the waveform.
    :param filename_prefix: Prefix for the output files (without extension).
    """
    # Save as .wav file
    wavfile.write(f"{filename_prefix}.wav", sample_rate, (wave * 32767).astype(np.int16))

    # Save as .json file
    json_data = wave.tolist()
    with open(f"{filename_prefix}.json", 'w') as json_file:
        json.dump(json_data, json_file)



# Default Usage, change as needed
if __name__ == "__main__":
    cycles = 2
    frequency = 100
    sample_rate = 44100
    amplitude = 0.5
    filename = 'incremental_wave'

    # Generate and save a sine wave
    wave_form = gen_incremental(sample_rate)
    save_waveform(wave_form, sample_rate, filename_prefix=filename)

