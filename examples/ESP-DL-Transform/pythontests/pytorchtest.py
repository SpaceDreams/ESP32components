import torch
import numpy as np
from KaldiCompliance import Kaldifbank
import matplotlib.pyplot as plt

fs = 48000
windowsamples = fs
windowstride = int(windowsamples/4)

## For 32-bit standard floats (4 bytes each)
#data_32 = np.fromfile("your_file.bin", dtype=np.float32)
#
## Reshape directly into the 3D dimensions
#matrix_3d = flat_array.reshape((DEPTH, ROWS, COLS))
#
## 3. Create a single figure with 4 subplots side-by-side
fig, axes = plt.subplots(2, 2, figsize=(16, 4))
#
#for i in range(4):
#    # Select the i-th slice [i, :, :]
#    slice_data = matrix_3d[i, :, :]
#    x_min, x_max = np.min(slice_data.min(axis=0)), np.max(slice_data.max(axis=0))
#    y_min, y_max = np.min(slice_data.min(axis=1)), np.max(slice_data.max(axis=1))
#    heatmap = plt.imshow(slice_data, extent=[x_min, x_max, y_min, y_max], aspect='auto')
#    # 3. Add a colorbar legend to decode the values
#    plt.colorbar(heatmap, label='Magnitude')
#

# Simulate a 1-second 48kHz audio stream with a 1kHz tone
t = np.linspace(0, windowsamples/fs, fs, endpoint=False)
t=t.reshape(len(t),1)
print(t.shape)
simulated_signal = lambda t,fq: np.sin(2 * np.pi * fq * t)
fbankclass= Kaldifbank(sample_frequency=fs,frame_length=0.02, frame_shift=0.01) 
fqtests=[100,1000,10000,23000]
signal_data = simulated_signal(t,fqtests[0])
for i,ax in enumerate(axes.flat,start=1):
    # Select the i-th slice [i, :, :]
    slice_data = fbankclass(signal_data).numpy()
    heatmap = ax.imshow(slice_data.T, extent=[i*windowstride/fs, 1+i*windowstride/fs, 0, 43], aspect='auto')
    if (i>2): continue
    #shift data similar to the esp32:
    signal_data[:(fs-windowstride),0] = signal_data[windowstride:,0]
    signal_data[(fs-windowstride):,0] = simulated_signal(t[0:windowstride,0]+i*windowstride/fs,fqtests[i])
# Add a single colorbar shared by the layout
fig.colorbar(heatmap, ax=axes.ravel().tolist(), shrink=0.6)
plt.show()
