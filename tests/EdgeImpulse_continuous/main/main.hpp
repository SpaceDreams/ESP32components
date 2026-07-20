

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "init_I2S.h"

/** Audio buffers, pointers and selectors */
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

inference_t inference;
const uint32_t sample_buffer_size = 2048;
signed short sampleBuffer[sample_buffer_size];
bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
bool record_status = true;

void audio_inference_callback(uint32_t n_bytes)
{
    for(int i = 0; i < n_bytes>>1; i++) {
        inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

        if(inference.buf_count >= inference.n_samples) {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
}

void capture_samples(void* arg) {

  const int32_t i2s_bytes_to_read = (uint32_t)arg;
  size_t bytes_read = i2s_bytes_to_read;

  while (record_status) {

    /* read data at once from i2s */
    i2s_read((i2s_port_t)1, (void*)sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);

    if (bytes_read <= 0) {
      ei_printf("Error in I2S read : %d", bytes_read);
    }
    else {
        if (bytes_read < i2s_bytes_to_read) {
        ei_printf("Partial I2S read");
        }

        // scale the data (otherwise the sound is too quiet)
        for (int x = 0; x < i2s_bytes_to_read/2; x++) {
            sampleBuffer[x] = (int16_t)(sampleBuffer[x]) * 8;
        }

        if (record_status) {
            audio_inference_callback(i2s_bytes_to_read);
        }
        else {
            break;
        }
    }
  }
  vTaskDelete(NULL);
}

void capture_samples()
{
    init_microphone();
    // Track whether the current 3-byte block is Left or Right
    //bool is_left_channel = true; 
    while (true) {
        // Read the RAW samples from the microphone
        if (i2s_channel_read(rx_handle, i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, portMAX_DELAY) == ESP_OK) {
            for (int i = 0; i < bytes_read; i+=3) { // Since I'm using a 24 bit mic I have to read in 3 bytes at a time for one sample
                int32_t value = 0;
                for (int j = 0; j<3; j++)
                    value |= i2s_readraw_buff[i+j]<<8*j;
                if (value & 0x00800000)
                       value |= 0xFF000000;
                printf("%ld\n",value);
            }
        } else {
            printf("Read Failed!\n");
        }
    }

    /* Have to stop the channel before deleting it */
    i2s_channel_disable(rx_handle);
    /* If the handle is not needed any more, delete it to release the channel resources */
    i2s_del_channel(rx_handle);
}
