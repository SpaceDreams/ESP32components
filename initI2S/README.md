For the ESP32s3 CRB Board the [ICS-43432](https://product.tdk.com/system/files/dam/doc/product/sw_piezo/mic/mems-mic/data_sheet/ics-43432-data-sheet-v1.3.pdf) Microphone is connected to the [TXB0104](https://www.ti.com/lit/ds/symlink/txb0104.pdf?ts=1781800524966&ref_url=https%253A%252F%252Fwww.mouser.com%252F) then the ESP32s3 pins:
1) SD -> IO2 Serial Data - pin 38 so GPIO 2
1) SCK -> IO41 Serial Clock / Bit Clock - pin 34 -> GPIO41 -> should be 64 x f_sampling
1) WS -> IO42 Serial Data Word Select - pin 35 -> GPIO42 -> should be 32bits long

pinout is found [here](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)

The LR Pin is OV which represents Left. According to the schematic it is connected to a solder joint whose default pin is ground. It can be configured for 2.8V to represent right.

## Data Word Length
The output data word length is 24 bits per channel. This is inside a 32 bit Slot structure.

## Data Word Format
The default data format is I²S (twos complement), MSB‐first. In this format, the MSB of each word is delayed by one SCK cycle from
the start of each half‐frame. Lastly, Word select is a 50% duty cycle. This is the standard Philips Format

## [How to Portion the DMA Buffer to Prevent Data Loss](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/api-reference/peripherals/i2s.html#how-to-prevent-data-lost)

1) Gather the data: `sample_rate`, `data_bit_width`, `slot_num` and `polling_cycle`.
   The polling cycle is how much time required to process the data after it is read. Two examples are: streaming data to a PC, saving data to a SD card. `slot_num` is the number of slots for one data collection cycle.
1) The maximum DMA buffer size is 4096; so I can calculate the 
```
dma_frame_num = dma_max_buffer_size/slot_num/data_bit_width*8
interrupt_interval = dma_frame_num / sample_rate
dma_desc_num > polling_cycle / interrupt_interval
recv_buffer_size > dma_desc_num * dma_buffer_size
```
1) [Note:](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/api-reference/peripherals/i2s.html#std-rx-mode) for a 24 bit ADC, `dma_frame_num`, `recv_buffer_size` and `mclk_multiple` should be a multiple of 3

