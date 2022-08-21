# low pass filter

import numpy as np
import matplotlib.pyplot as plt
import scipy.signal as signal


def lowpass(x, cutoff, fs, order=5):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = signal.butter(order, normal_cutoff, btype='low', analog=False)
    return signal.filtfilt(b, a, x)

def draw_2_signals(x, y, x2, y2, title, xlabel, ylabel):
    plt.plot(x, y)
    plt.plot(x2, y2)
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.show()

def draw_signal(x, y, title, xlabel, ylabel, filename):
    plt.plot(x, y)
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.savefig(filename)
    plt.show()

def generate_noise(length, amplitude):
    return amplitude * np.random.randn(length)

if(__name__ == "__main__"):
    fs = 100
    cutoff = 3
    length = 1000
    amplitude = 1
    x = np.linspace(0, length, length)
    y = generate_noise(length, amplitude)
    y_filtered = lowpass(y, cutoff, fs)
    draw_2_signals(x, y, x, y_filtered, "Noise", "Time", "Amplitude")
    print("Done")