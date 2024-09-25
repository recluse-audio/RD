import unittest
import wave
import numpy as np
from scipy.signal import find_peaks
# Import the sine wave generation function
from gen_wave import gen_sin_wave, gen_incremental

class TestWave(unittest.TestCase):

    # Define the path to the .wav file that was written
    wav_file_path = '../WAVEFORMS/incremental_wave.wav'  

    def test_number_of_samples(self):
        cycles = 5
        frequency = 440
        sample_rate = 44100
        wave = gen_sin_wave(cycles, frequency, sample_rate)
        
        expected_duration = cycles / frequency
        expected_samples = int(sample_rate * expected_duration)
        self.assertEqual(len(wave), expected_samples, "Number of samples does not match the expected duration.")

    def test_amplitude_values(self):
        cycles = 5
        frequency = 440
        sample_rate = 44100
        amplitude = 0.5
        wave = gen_sin_wave(cycles, frequency, sample_rate, amplitude)
        
        self.assertTrue(np.all(wave <= amplitude) and np.all(wave >= -amplitude), 
                        "Amplitude values exceed the specified range.")

    def test_zero_crossings(self):
        cycles = 5
        frequency = 440
        sample_rate = 44100
        wave = gen_sin_wave(cycles, frequency, sample_rate)
        
        zero_crossings = np.where(np.diff(np.sign(wave)))[0]
        self.assertEqual(len(zero_crossings), 2 * cycles, "Incorrect number of zero crossings for the given cycles.")

    def test_frequency(self):
        cycles = 5
        frequency = 440
        sample_rate = 44100
        wave = gen_sin_wave(cycles, frequency, sample_rate)
        
        peaks, _ = find_peaks(wave)
        periods = np.diff(peaks) / sample_rate
        measured_frequency = 1 / np.mean(periods)
        
        self.assertAlmostEqual(measured_frequency, frequency, delta=1, 
                               msg=f"Measured frequency {measured_frequency} does not match expected frequency {frequency}.")

    def test_incremental(self):
        wave = gen_incremental(100)
        for i in range(len(wave)):
            self.assertAlmostEqual(wave[i], float(i))




if __name__ == "__main__":
    unittest.main()
