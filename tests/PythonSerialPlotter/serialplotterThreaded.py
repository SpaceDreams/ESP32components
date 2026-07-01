import serial
import matplotlib.pyplot as plt
import threading
import queue
from collections import deque

def is_integer(string):
    allints = []
    for i in string:
        try:
            int(string)
            allint.append(True)
        except ValueError:
            allint.append(False)
    return all(allints)
# Thread-safe queue to pass data between serial thread and plot thread
data_queue = [queue.Queue() for _ in range(4)]
running = True

def read_serial(port, baudrate):
    global running
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        while running:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip().split(",")
                if are_integers(line):
                    for (i in range(len(line)))
                        try:
                            value = int(line)
                            data_queue[i].put(value)
                        except ValueError:
                            pass
                else:
                    print(line)
    except Exception as e:
        print(f"Serial error: {e}")
    finally:
        ser.close()

# Start the background thread
# Replace 'COM3' with '/dev/ttyACM0' or '/dev/cu.usbserial' depending on your OS
serial_thread = threading.Thread(target=read_serial, args=('COM7', 9600))
serial_thread.start()

# Matplotlib setup
plt.ion()
fig, ax = plt.subplots(2,2)
line, = ax.plot([], [], '-')
data_history = deque(maxlen=100) # Keeps a moving window of 100 points

try:
    while True:
        # Pull data from the thread-safe queue
        while not data_queue.empty():
            val = data_queue.get()
            data_history.append(val)
            
        # Update the plot if we have data
        if data_history:
            line.set_xdata(range(len(data_history)))
            line.set_ydata(data_history)
            ax.set_xlim(0, len(data_history))
            ax.relim()
            ax.autoscale_view()
            
        # Use plt.pause() instead of time.sleep() to keep the GUI responsive
        plt.pause(0.01)
except KeyboardInterrupt:
    running = False
    serial_thread.join()
