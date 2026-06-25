import serial
import matplotlib.pyplot as plt
from collections import deque


def is_integer(string):
    try:
        int(string)
        return True
    except ValueError:
        return False

# Configure your port (Change 'COM3' to your respective port name)
ser = serial.Serial('COM7', 9600, timeout=1)
data_history = deque(maxlen=100) # Keeps a moving window of 100 points

plt.ion() # Turn on interactive mode
fig, ax = plt.subplots()
line, = ax.plot(data_history)

while True:
    try:
        raw_line = ser.readline().decode('utf-8').strip()
        if is_integer(raw_line): # Ensure data is numerical
            data_history.append(int(raw_line))
            line.set_ydata(data_history)
            line.set_xdata(range(len(data_history)))
            ax.set_xlim(0, len(data_history))
            ax.relim()
            ax.autoscale_view()
            plt.pause(0.01)
        else:
            print(raw_line)
    except KeyboardInterrupt:
        break

ser.close()