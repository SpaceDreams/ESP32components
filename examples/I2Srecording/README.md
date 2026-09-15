# Usage
This is a saving audio example. Tested successfully on Sep. 15, 2026

# Usage Optimization
For High Frequency Recordings it is benefical to reserve space on the sd card prior to recording since recording can cause delays. The following code does that for a standard wav file with a 44 byte header.:

```
FILE* init_wavfile(uint32_t rec_time, const char *filename)
{
    FILE* f = init_file(filename);
    
    // 1. Pre-allocate the entire estimated size (e.g., 10 MB total)
    uint8_t wavheadersize = 44; //bytes
    uint32_t total_file_size = rec_time*INIT_AUDIO_SAMPLE_RATE*(AUDIO_BIT_WIDTH/8)+wavheadersize;
    fseek(f, total_file_size - 1, SEEK_SET);
    fputc(0, f);
    
    // 2. Rewind to the beginning
    fseek(f, 0, SEEK_SET);
    
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(BYTE_RATE * rec_time, AUDIO_BIT_WIDTH, INIT_AUDIO_SAMPLE_RATE, NUM_CHANNELS);
    // Write the header to the WAV file
    fwrite(&wav_header, sizeof(wav_header), 1, f);
    return f;
}
```

The next option is to avoid using fwrite and use lower level functions like write and create blocks of data. The above is a little simpler and seems to work well with 48kHz.