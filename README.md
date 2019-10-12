# ESP-Power-Monitoring
Power Monitoring based on ESP8266 and ATM90E32


## Hardware
ATM90E32 is a poly-phase energy metering ic, so 3 lines can be monitor

This board can be work in 2 mode :
  
  * 1 phase mode : 110V to 240V AC (up to 3 line)
  * 3 phase mode : 380V AC (use the 3 line)
 
[Here the schematic of this board](Hardware/Project%20Outputs%20for%20ESP-Power-Monitoring/Schematic%20Print/Schematic%20Prints.PDF)

And some pictures of this board

![top](Doc/png/2019-10-12%2010_16_24-ESP-Power-Monitoring.PrjPcb%20-%20Altium%20Designer%20(19.1.8).png)

![bottom](Doc/png/2019-10-12%2010_17_01-ESP-Power-Monitoring.PrjPcb%20-%20Altium%20Designer%20(19.1.8).png)

This board can be mount on DIN rail

![din rail enclosure](Doc/png/2019-10-12%2010_14_08-ESP-Power-Monitoring.PrjPcb%20-%20Altium%20Designer%20(19.1.8).png)

## Software
Send data in MQTT at a fix configurable time 
