import torch
import numpy as np
from KaldiCompliance import Kaldifbank
import matplotlib.pyplot as plt

fs = 48000
windowsamples = fs
windowstride = int(windowsamples/4)

# For 32-bit standard floats (4 bytes each)
#data_32 = np.fromfile("simulated_signal_r5.bin", dtype=np.float32)
data_32 = np.fromfile("data_1.bin", dtype=np.float32)
matrix_3d=np.empty(((5, 99, 40)), dtype=np.float32)
# Reshape directly into the 3D dimensions
matrix_3d[0,:,:] = data_32.reshape((1, 99, 40))

# 3. Create a single figure with 4 subplots side-by-side
fig, axes = plt.subplots(2, 3, figsize=(16, 4))

for i,ax in enumerate(axes.flat,start=0):
    if i>4: continue 
    # Select the i-th slice [i, :, :]
    slice_data = matrix_3d[i, :, :]
    heatmap = ax.imshow(slice_data.T, extent=[i*windowstride/fs, 1+i*windowstride/fs, 40, 0], aspect='auto')
    # 3. Add a colorbar legend to decode the values
fig.colorbar(heatmap, ax=axes.ravel().tolist(), shrink=0.6)

fig1, axes1 = plt.subplots(2, 3, figsize=(16, 4))
fig2, axes2 = plt.subplots(2, 3, figsize=(16, 4))
# Simulate a 1-second 48kHz audio stream with a 1kHz tone
t = np.linspace(0, windowsamples/fs, fs, endpoint=False)
t=t.reshape(len(t),1)
print(t.shape)
simulated_signal = lambda t,fq: np.sin(2 * np.pi * fq * t)
fbankclass= Kaldifbank(sample_frequency=fs,frame_length=0.02, frame_shift=0.01) 
fqtests=[100,1000,10000,20000,23000]
slice_data=np.empty(((5, 99, 40)), dtype=np.float32)


for i,ax in enumerate(axes1.flat,start=0):
    if  i>4: continue
    # Select the i-th slice [i, :, :]
    if i<1: signal_data = simulated_signal(t+i*windowstride/fs,fqtests[i])
    else:  signal_data[windowsamples-windowstride:]=simulated_signal(t[:windowstride]+i*windowstride/fs,fqtests[i])
    print("Signal Data Shape: ",signal_data.shape)
    slice_data = fbankclass(signal_data).numpy()
    heatmap = ax.imshow(slice_data.T, extent=[i*windowstride/fs, 1+i*windowstride/fs, 40, 0], aspect='auto')
    print(slice_data.shape)
    squared_errors = (slice_data - matrix_3d[i,:,:]) ** 2
    # Calculate the variance of these errors
    error_variance = np.var(squared_errors)
    # Calculate the standard deviation of these errors
    error_std_dev = np.std(squared_errors)
    print(f"Total MSE for time sample {i}: {error_std_dev} +/- {error_variance}") 
    #shift the data, like on the esp32:
    signal_data[:windowsamples-windowstride] =signal_data[windowstride:] 
    heatmap1 = axes2.flat[i].imshow(np.log10(squared_errors.T), extent=[i*windowstride/fs, 1+i*windowstride/fs, 40, 0], aspect='auto')
    #if (i>2): continue
    #shift data similar to the esp32:
    #signal_data[:(fs-windowstride),0] = signal_data[windowstride:,0]
    #signal_data[(fs-windowstride):,0] = simulated_signal(t[0:windowstride,0]+i*windowstride/fs,fqtests[i])
# Add a single colorbar shared by the layout
fig1.colorbar(heatmap, ax=axes1.ravel().tolist(), shrink=0.6)
fig2.colorbar(heatmap1, ax=axes2.ravel().tolist(), shrink=0.6)
plt.show()
