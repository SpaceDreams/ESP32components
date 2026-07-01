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
