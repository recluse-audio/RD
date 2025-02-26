import numpy as np
import matplotlib.pyplot as plt
import csv
import argparse

def plot_audio_csv(csv_file, sample_rate=44100, downsample_factor=10):
    """
    Reads a CSV file containing audio samples, downsamples, and plots an audio waveform thumbnail.

    Args:
        csv_file (str): Path to the CSV file.
        sample_rate (int, optional): Sampling rate of the audio data. Default is 44100 Hz.
        downsample_factor (int, optional): Factor by which to downsample the data. Default is 10.
    """
    audio_data = []

    # Read CSV file, skipping the first row (headers) and first column (sample index)
    with open(csv_file, newline='') as file:
        reader = csv.reader(file)
        headers = next(reader)  # Skip the first row (header)
        print(f"Headers: {headers}")  # Debugging: Print headers

        for row in reader:
            audio_data.append([float(sample) for sample in row[1:]])  # Skip first column

    audio_data = np.array(audio_data)

    # Limit to the first 2000 indices
    audio_data = audio_data[:2000]

    # Downsample the data
    audio_data = audio_data[::downsample_factor]

    # Check if mono or stereo
    if audio_data.shape[1] == 1:
        mono_audio = audio_data[:, 0]
        stereo = False
    else:
        stereo = True
        left_channel = audio_data[:, 0]
        right_channel = audio_data[:, 1]

    # Generate time axis based on downsampled audio data
    duration = len(audio_data) * downsample_factor / sample_rate
    time_axis = np.linspace(0, duration, len(audio_data))



    # Create subplots for each channel
    plt.figure(figsize=(12, 6))

    if stereo:
        # Left channel plot
        plt.subplot(2, 1, 1)  # 2 rows, 1 column, first subplot
        plt.plot(time_axis, left_channel, label="Left Channel", alpha=0.7)
        plt.title("Left Channel")
        plt.xlabel("Time (seconds)")
        plt.ylabel("Amplitude")
        plt.ylim([-1, 1])  # Set y-axis limits to show normalized amplitude
        plt.grid(True, linestyle="--", alpha=0.5)

        # Right channel plot
        plt.subplot(2, 1, 2)  # 2 rows, 1 column, second subplot
        plt.plot(time_axis, right_channel, label="Right Channel", alpha=0.7, color='orange')
        plt.title("Right Channel")
        plt.xlabel("Time (seconds)")
        plt.ylabel("Amplitude")
        plt.ylim([-1, 1])  # Set y-axis limits to show normalized amplitude
        plt.grid(True, linestyle="--", alpha=0.5)

    else:
        plt.plot(time_axis, mono_audio, label="Mono Audio", alpha=0.7)
        plt.title("Mono Audio")
        plt.xlabel("Time (seconds)")
        plt.ylabel("Amplitude")
        plt.ylim([-1, 1])  # Set y-axis limits to show normalized amplitude
        plt.grid(True, linestyle="--", alpha=0.5)

    plt.tight_layout()  # Adjust layout to prevent overlap
    plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Visualize an audio CSV file as a waveform.")
    parser.add_argument("csv_path", type=str, help="Path to the CSV file containing audio data")
    parser.add_argument("--downsample", type=int, default=10, help="Factor to downsample the audio data (default: 10)")
    args = parser.parse_args()

    plot_audio_csv(args.csv_path, downsample_factor=args.downsample)
