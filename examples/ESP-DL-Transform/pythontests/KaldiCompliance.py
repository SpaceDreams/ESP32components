import torch
import torch.nn as nn
import torchaudio.compliance.kaldi as kaldi

class KaldiSpectrogram(nn.Module):
    def __init__(self, sample_frequency=48000.0, frame_length=25.0, frame_shift=10.0):
        super().__init__()
        self.sample_frequency = sample_frequency
        self.frame_length = frame_length
        self.frame_shift = frame_shift

    def forward(self, waveform: torch.Tensor) -> torch.Tensor:
        torchwaveform = torch.from_numpy(waveform).float().T
        # kaldi.spectrogram expects a 2D tensor of shape (channel, time)
        res= kaldi.spectrogram(
            torchwaveform,
            sample_frequency=self.sample_frequency,
            frame_length=self.frame_length,
            frame_shift=self.frame_shift
        )
        return res

class Kaldifbank(nn.Module):#Mel-filterbank
    def __init__(self, sample_frequency=48000.0,frame_length=0.02, frame_shift=0.01,filter_number=40,
                 low_frequency=0,high_frequency=0,noise_floor=-52):
        super().__init__()
        self.sample_frequency = sample_frequency
        self.frame_length = frame_length#Units are seconds
        self.frame_shift = frame_shift#Units are seconds
        self.filter_number = filter_number
        self.low_frequency=low_frequency
        self.high_frequency=high_frequency
        self.noise_floor = noise_floor #in dB

    def forward(self, waveform: torch.Tensor) -> torch.Tensor:
        torchwaveform = torch.from_numpy(waveform).float().T
        # the parsing the wave file provides (time,channel)
        # kaldi.spectrogram expects a 2D tensor of shape (channel, time)
        res = kaldi.fbank(
            torchwaveform,
            sample_frequency=self.sample_frequency,
            frame_length=self.frame_length*1000,#Units are milliseconds
            frame_shift=self.frame_shift*1000,#Units are milliseconds
            num_mel_bins=self.filter_number,
            low_freq=self.low_frequency,
            high_freq=self.high_frequency,
            #energy_floor = 10**(self.noise_floor/10), #noise_floor is in dB where energy_floor is not
            energy_floor = 0,#The ESP-DL library sets this at 0. 
            round_to_power_of_two=True,
            window_type="hamming",
            dither=0.0,
            preemphasis_coefficient=0.0
        )
        # 1. Hard clamp extreme values to stabilize the distribution boundary
        # Typical the faucet fbank features sit well between -20 and 80 dB
        fbank = torch.clamp(res, min=-20.0, max=80.0)
        # 2. Normalize to a predictable, tight range (e.g., -1.0 to 1.0)
        # This prevents the Power-of-Two scale from leaving huge empty gaps in your INT8 range
        fbank_normalized = 2.0 * (fbank - (-20.0)) / (80.0 - (-20.0)) - 1.0
        return fbank_normalized 

class KaldiMFCC(nn.Module):
    def __init__(self, sample_frequency=48000.0,n_of_coeffs=13,frame_length=0.02, frame_shift=0.01,
                 filter_number=32, low_frequency=0, high_frequency=0,preemphasis_coefficient=0.98):
        super().__init__()
        self.sample_frequency = sample_frequency
        self.n_of_coeffs=n_of_coeffs
        self.frame_length = frame_length#Units are seconds
        self.frame_shift = frame_shift#Units are seconds
        self.filter_number = filter_number
        self.low_frequency = low_frequency
        self.high_frequency = high_frequency
        self.preemphasis_coefficient = preemphasis_coefficient #in dB

    def forward(self, waveform: torch.Tensor) -> torch.Tensor:
        torchwaveform = torch.from_numpy(waveform).float().T
        # kaldi.spectrogram expects a 2D tensor of shape (channel, time)
        res= kaldi.mfcc(
            torchwaveform,
            sample_frequency=self.sample_frequency,
            num_ceps=self.n_of_coeffs,
            frame_length=self.frame_length*1000,#Units are milliseconds
            frame_shift=self.frame_shift*1000,#Units are milliseconds
            num_mel_bins=self.filter_number,
            low_freq=self.low_frequency,
            high_freq=self.high_frequency,
            preemphasis_coefficient = self.preemphasis_coefficient,
            window_type="hamming",      # Force Hamming match
            dither=0.0                  # Turn off Kaldi's random noise element
        )
        return torch.transpose(res, 0, 1) # This places time on the 3rd dim, compliant with esp-dl streaming