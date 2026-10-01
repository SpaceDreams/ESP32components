1) SDCard Pinout:

CLK     --> IO47

DAT0    --> IO39

DAT1    --> IO40

DAT2    --> IO0

DAT3/CD --> IO38

CMD     --> IO48

2) ADC PIN: https://documentation.espressif.com/esp32-s3_datasheet_en.pdf#cd-pins-peri-assignment

ADC1: GPIO1-10

ADC2: GPIO11-14, XTAL_32K_P (Pin 21), XTAL_32K_N (Pin 22), GPIO17-20

(Note: GPIO1-10 means GPIO1, GPIO2,...,GPIO10)

3) USB is directly connected to esp32-23:
https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/get-started/windows-start-project.html

# Reminders 
-set the build target:
idf.py set-target esp32s3
## Setup USB serial/JTAG controller setting
1) idf.py menuconfig
2) Component config
3) ESP-STDIO
4) USB Serial/JTAG Controller
more information can be found here: 
https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/api-guides/usb-serial-jtag-console.html
and here:
https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/api-reference/kconfig-reference.html#config-esp-console-uart

## Setup Custom UART

To use UART on the CRB board it requires the configuration below.

1) idf.py menuconfig
2) Component config
3) ESP-STDIO
4) Custom UART

Transmit pin -> pin 37 -> GPIO 43

Receive pin -> pin 36 -> GPIO 44

Baud rate is 115200 -- This is the default

Pinout and datasheet is [here](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf):


