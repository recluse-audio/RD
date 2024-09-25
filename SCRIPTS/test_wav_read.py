import unittest
import wave
import numpy as np

class TestIncrementalWavFile(unittest.TestCase):
    def setUp(self):
        # Path to the .wav file that was written
        self.wav_file_path = '../WAVEFORMS/incremental_wave.wav'  # Replace with your actual file path

        # Expected properties
        self.expected_sample_rate = 44100
        self.expected_channels = 1

    def test_wav_file_exists(self):
        # Check if the .wav file exists
        try:
            with wave.open(self.wav_file_path, 'rb') as wav_file:
                pass
        except FileNotFoundError:
            self.fail(f"File {self.wav_file_path} does not exist.")

    def test_wav_file_properties(self):
        # Open the .wav file to check properties
        with wave.open(self.wav_file_path, 'rb') as wav_file:
            # Check number of channels
            self.assertEqual(wav_file.getnchannels(), self.expected_channels, "Number of channels does not match.")
            
            # Check sample rate
            self.assertEqual(wav_file.getframerate(), self.expected_sample_rate, "Sample rate does not match.")


if __name__ == "__main__":
    unittest.main()
