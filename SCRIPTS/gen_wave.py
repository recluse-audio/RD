import unittest
import numpy as np
from scipy.io import wavfile  # Correct import for wavfile
import json

# This is a general purpose set of functions with pytest coverage for generating .wav and .json files in the shape of waves or whatever
# Still learning how to organize my python code.  Apologies.  
# 
# Workflow is this: Modify the variables and functions calls in the main function at the bottom.  
# Maybe I'll make this accept args in the future, but for now not gonna

####################
def gen_sin_wave(cycles, frequency, sample_rate=44100, amplitude=0.5):
    duration = cycles / frequency
    t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
    return amplitude * np.sin(2 * np.pi * frequency * t)

##################
def gen_sin_wave_with_period(period, cycles=1, amp=0.5):
    """
    Generate a sine wave based on the period length in samples and number of cycles.

    Args:
    - period (int): The length of one sine wave cycle in samples.
    - cycles (int): Number of cycles to generate.
    - amp (float): The amplitude of the sine wave.

    Returns:
    - np.ndarray: Array of sine wave values.
    """
    total_samples = period * cycles
    t = np.linspace(0, cycles, total_samples, endpoint=False)
    return amp * np.sin(2 * np.pi * t)


#################
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
    # wavfile.write(f"{filename_prefix}.wav", sample_rate, (wave * 32767).astype(np.int16))

    # Save as .json file
    json_data = wave.tolist()
    with open(f"{filename_prefix}.json", 'w') as json_file:
        json.dump(json_data, json_file)


#####################
# Saves the wave_form as a json, creates a new column at each specified period
# Pass in a 256 sample long wave_form that is a sine wave with a period of 128 samples long - two cycles in total
# this writes the values of each corresponding phase index of the sine wave side by side.
# [0] - [128]
# [1] - [129]
# ... 
# [127] - [255]
# if cyclical these should be identical
def save_json_columns(wave_form, period):
    # Split the sine wave into two columns
    first_cycle = wave_form[:period]  # Index 0 to 127
    second_cycle = wave_form[period:period*2]  # Index 128 to 255

    # Combine into a side-by-side structure for JSON
    side_by_side = [{"Cycle One Sample Val": float(first_cycle[i]), "Cycle Two Sample Val": float(second_cycle[i])} for i in range(period)]

    # Convert to JSON and save
    json_output = json.dumps(side_by_side, indent=4)

    # Save the JSON to a file (optional)
    with open("sine_wave_side_by_side.json", "w") as f:
        f.write(json_output)




# Default Usage, change as needed
if __name__ == "__main__":
    cycles = 2
    period = 128
    sample_rate = 44100
    amplitude = 0.5
    filename = 'sine_128samples_2cycles_2columns'

    # Generate and save a sine wave
    wave_form = gen_sin_wave_with_period(period, cycles)
    save_json_columns(wave_form, period)
    #save_waveform(wave_form, sample_rate, filename_prefix=filename)

