

#include "dl_fbank.hpp"
#include "math.h" //need pi use: M_PI
#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"//Necessary for clock to time conversion
#include "freertos/task.h" //Necessary for delays
// Transform Configuration:
#define NUM_MEL_BINS        40 //
#define FRAME_LENGTH        20 //[ms] in milliseconds
#define FRAME_SHIFT         10 //[ms]
#define SAMPLE_RATE         48000
#define WINDOWSAMPLES      SAMPLE_RATE // This is set by the model
#define WINDOWSTRIDE        (WINDOWSAMPLES/4) //The minimum number is (frame_length*Sample_Rate)
#define GET_transformXsize(framesamples) (((framesamples)-(FRAME_LENGTH))/(FRAME_SHIFT)+1)
#define OVERLAP           ((FRAME_LENGTH-FRAME_SHIFT)*(WINDOWSAMPLES/1000))//
#define INITSTRIDESHAPE_X   GET_transformXsize((WINDOWSTRIDE/(WINDOWSAMPLES/1000)))
#define STRIDESHAPE_X       GET_transformXsize((WINDOWSTRIDE/(WINDOWSAMPLES/1000))+FRAME_LENGTH-FRAME_SHIFT)
#define INITSTRIDESHAPE_Y   NUM_MEL_BINS
#define STRIDESHAPE_Y       NUM_MEL_BINS
//transformoutput will be bigger than what's needed for the first iteration, so here I set an offset:
#define OFFSET              ((STRIDESHAPE_X)*(STRIDESHAPE_Y) - (INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y))

static const char *TAG = "Transform Tests";
// Transform Configuration
dl::audio::Fbank * transform=nullptr;
static const uint16_t sizeoftransout = (STRIDESHAPE_X)*(STRIDESHAPE_Y);//25*40
static const uint16_t sizeofoverlapbuff = WINDOWSTRIDE+OVERLAP;//12000+480

static float transformoutput[sizeoftransout]={0};
// not sure the best way to handle the overlap; here I just make an array
static float overlapbuff[sizeofoverlapbuff]={0};
static float transforminput[WINDOWSTRIDE]={0};

__attribute__((const)) float normalize(const float x){
	//# 1. Hard clamp extreme values to stabilize the distribution boundary
    //# Typical faucet fbank features sit well between -20 and 80 dB
	float min_val = -20.0f;
    float max_val = 80.0f;
    float res = x;
	if (x>max_val)
		res = 80.0f;
	else if(x<min_val)
		res=-20.0f;
	//# 2. Normalize to a predictable, tight range (e.g., -1.0 to 1.0)
    //# This prevents the Power-of-Two scale from leaving huge empty gaps in the INT8 range
    //fbank_normalized = 2.0 * (fbank - (-20.0)) / (80.0 - (-20.0)) - 1.0
    //fbank_normalized = (fbank - (-20.0)) / 50 - 1.0 = fbank/50+2/5-1=fbank/50-3/5
    float multiplier = 0.02f;
    float subtract_val = 0.6f;
    return res*multiplier-subtract_val;
}

void inittransform(float * input, float * output){
    memcpy(overlapbuff,&input[WINDOWSTRIDE-OVERLAP],OVERLAP*sizeof(input[0]));
    transform->process(input, WINDOWSTRIDE, output);
    for (size_t i = 0; i < (INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y); i++)
        output[i] = normalize(output[i]);
}
void slicetransform(float * input, float * output){
    memcpy(&overlapbuff[OVERLAP],input,WINDOWSTRIDE*sizeof(input[0]));
    transform->process(overlapbuff, WINDOWSTRIDE+OVERLAP, output);
    memmove(overlapbuff,&overlapbuff[WINDOWSTRIDE],OVERLAP*sizeof(overlapbuff[0]));
    for (size_t i = 0; i < (STRIDESHAPE_X)*(STRIDESHAPE_Y); i++)
        output[i] = normalize(output[i]);
}

bool transform_init(){
    /** Audio buffers, pointers and selectors **/
    // Transform Configuration
    dl::audio::SpeechFeatureConfig config;
    config.sample_rate = SAMPLE_RATE;
    config.frame_length = FRAME_LENGTH;  // ms
    config.frame_shift = FRAME_SHIFT;   // ms
    config.num_mel_bins = NUM_MEL_BINS;
    //config.energy_floor = pow(2,(NOISE_FLOOR/10f)-24)*pow(5,(NOISE_FLOOR/10f));//10**(NOISE_FLOOR/10f)/2**24
    //config.round_to_power_of_two=true;
    config.window_type = dl::audio::WinType::HAMMING;
    //config.dither=0.0;
    config.preemphasis=0.0;
    transform = new dl::audio::Fbank(config);
    if(transform == nullptr){
            ESP_LOGE(TAG, "Error, failed to assign model pointers!");
            return false;
    }
    return true;
}

extern "C" void app_main(void){
    // Configure USB SERIAL JTAG
    usb_serial_jtag_driver_config_t usb_serial_jtag_config = {
        .tx_buffer_size = 1024,//These don't have to be large as long as reading/writing is done quickly
        .rx_buffer_size = 1024,
    };
    uint8_t ack = 0x06;
    ESP_LOGI(TAG, "USB_SERIAL_JTAG init done");

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_serial_jtag_config));
    // Now I can initalize the classifier
    if (transform_init() == false) {
        ESP_LOGE(TAG, "Error Initializing Transform");
        return;
    }

    while(true)
    {
        // 1.1. Read the 3-byte length header
        uint8_t header[3] = {0};
        int len_read = usb_serial_jtag_read_bytes(header, 3, 2000 / portTICK_PERIOD_MS);
        uint16_t payload_len = (((uint16_t)header[2])<<8)|header[1];
        uint8_t init = header[0]; // This is the initializer; it's 0x00 if the first chunk of data
        if (len_read != 3 || payload_len > sizeof(float)*WINDOWSTRIDE)
            continue;
        ESP_LOGI(TAG, "Total payload length: %d",payload_len);
        //fflush(stdout);
        //vTaskDelay(2 / portTICK_PERIOD_MS);
        usb_serial_jtag_write_bytes(&ack, 1, 2000 / portTICK_PERIOD_MS);
        // 2. Read the chunk payload fully
        uint16_t total_received = 0;
        while ( total_received < payload_len ) {
            // 1. Guard against silent overflow
            if (payload_len > sizeof(float)*WINDOWSTRIDE) {
                printf("ERROR: Overflow risk! Payload size (%d bytes) is larger than buffer (%d bytes)\n", 
                    payload_len, sizeof(float)*WINDOWSTRIDE);
                fflush(stdout);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                return; // Abort or handle the error safely
            }
            int n = usb_serial_jtag_read_bytes(
                ((uint8_t *)transforminput) + total_received, 
                payload_len - total_received, 
                10000 / portTICK_PERIOD_MS
            );
            if (n <= 0){
                ESP_LOGI(TAG, "Read timeout or disconnected");
                fflush(stdout);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                break;
            }
            total_received += n;
        }
        ESP_LOGI(TAG, "Total Recieved bytes: %d",total_received);
        //fflush(stdout);
        //vTaskDelay(2000 / portTICK_PERIOD_MS);
        if (total_received == payload_len) {
            // Send ACK back to Python
            usb_serial_jtag_write_bytes(&ack, 1, 2000 / portTICK_PERIOD_MS);
            uint16_t size_out=(STRIDESHAPE_X)*(STRIDESHAPE_Y);
            if(init != 0x00){
                inittransform(transforminput,transformoutput);
                size_out=(INITSTRIDESHAPE_X)*(INITSTRIDESHAPE_Y);
            }
            else
                slicetransform(transforminput,transformoutput);
            // 1. Send the 2-byte length header first
            usb_serial_jtag_write_bytes((uint8_t *)&size_out, sizeof(size_out), 2000 / portTICK_PERIOD_MS);
            uint8_t rxAck=0;
            usb_serial_jtag_read_bytes(&rxAck, 1, 2000 / portTICK_PERIOD_MS);
            if (rxAck!=ack)
                ESP_LOGI(TAG, "Python has not Acknoledged");
            // 2. Send the actual float data payload
            usb_serial_jtag_write_bytes((uint8_t *)transformoutput, size_out*sizeof(transformoutput[0]), 10000 / portTICK_PERIOD_MS);
            ESP_LOGI(TAG, "Total Transmitted bytes: %d",size_out*sizeof(transformoutput[0]));
        }
    }
}