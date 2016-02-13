#Mutatio Instructions

##Uploading Firmware
Install the Arduino IDE (1.6.7 or later) and add the ESP8266 extension by following the instructions given [here](https://github.com/esp8266/Arduino). I prefer the manual cloning method over the boards manager since it works better for me. If you get any weird compiler errors, try installing manually.
The Mutatio firmware only uses two external libraries. One can be installed using the Arduino Library Manager. Install the Adafruit NeoPixel from the Manager and manually install the RTC library that can be found [here](https://github.com/Makuna/Rtc).
After installing the libraries the code should compile without errors.
To upload the code to the Mutatio, set the board settings to:

![settings](/Hardware/Mutatio/Pictures/uploadsettings.jpg)

Note: after setting up the WiFi connection also over the air uploading using WiFi (OTA update) instead of serial can be used.

##Setup
The first thing should be to get a proper signal. For testing, a 50cm (or longer) cable can be connected to the external antenna point marked 'Antenna' on the board. I use wire wrap wire but any wire will do. The Mutatio sometimes does not boot properly if there is no signal present, it is unknown why.
Open the serial port using the Arduino IDE serial monitor or any other serial monitor. I prefer 'Realterm' since it runs more stable on Windows than the Arduino monitor. Setting the Baudrate to 115200 and then resetting the board (press reset button, repeat if not booting) there should be debug output on the serial. The first two lines of gibberish are debug info from the ESP8266 chip itself and can be ignored.
Then the output should look like this:
```
sldœŸ|Œlà|Œläb|Žƒä›{›cŒcŒòggžl'nœãìcpä$slslxògàƒd„œcoâ|lŒŒ‡ÇcŒógnçd„d`˜ggl ns
››oƒŒl`r’“gƒ„d`üƒoœ                                                             
Mutatio grid frequency tracker, part of netzsin.us project                      
Reading Configuration                                                           
Configurarion Found!                                                            
connecting to MYSSID                                                            
WIFI not available: NO SSID AVAILBLE  
```
Next the Mutatio needs to be configured. This is done through the setup page. To access it, press and hold the 'Flash' button for three seconds, then release it. The LED color turns purple indicating that the accesspoint is ready. Connect your computer's WiFi to the accesspoint named 'Mutatio-XXXX' with the default password '11111111'. Open any web browser and go to '192.168.4.1'. The config page will show. Enter your routers SSID and password. As a default, DHCP is used to obtain an IP. For a static IP you can also disable DHCP and enter the connection addresses manually.
The second part of the page lets you configure the Mutatio hardware and server settings.
Once the Mutatio is connected to your local WiFi the configuration page can also be accessed by entering the Muatio's IP local IP address: check the serial output for the IP or assign a static IP.

##LED status colors
The LED colors indicate the status:
- **Red**: no frequency signal available: check antenna and bring a power cable close to it. If using the on board antenna: make sure the Mutatio is mounted well on the power cable as shown in the [photo](Hardware/Mutatio/Pictures/Mutatio_REV1_finished_backview.jpg).
- blinking **Yellow**: WiFi disconnected: after booting it takes up to 20s to connect
- blinking **Blue**: SD card access failed: check if SD card is present and FAT formatted 
- bright purple: access point running, 5 minute timeout 
- dim purple: OTA update in progress. Do not disconnect power or update will fail and may require re-upload through USB
- during normal operation the LED indicates the grid frequency: from blue (-60mHz) to green (0mHz) to red (+60mHz)

##Problem solving
######Frequency Signal
To check proper operation use the serial monitor. If the frequency signal is available and the time is properly set (either from the RTC or from an NTP server) there will be one measurement value displayed each second. It looks like this
```
todo: insert line here
```
The first number is the frequency offset calculated. The second value is the moving average over all measurement values, including all raw values. This should be very close or even identical to the first value on a good signal. The third value is a quality indicator: it shows how many measurement points out of 100 values are regarded as valid: to calculate the frequency 100 values are measured and averaged. Outliers are discarded as noise. If this number is lower than 90 the input signal is too noisy. Try a shorter antenna and bring it as close as possible to the power cable: a 15cm antenna wrapped around the cable works nicely but the best signal is obtained from the onboard antenna.

######Time
The time is constantly checked against a NTP server ('pool.ntp.org' by default). If the connection is too slow (average ping > 30ms) an accurate time sync is not possible. The accepted latancy can be changed in the firmware but it is not recommended since it leads to time jitter. To do so change 
````
#define ALLOWEDROUNDTRIPDELAY 30
```
in NTP_Time.h to a higher number.

To change the time server (or server pool) edit the string 
```
const char* ntpServerName = "pool.ntp.org";
```
also in in NTP_Time.h

The RTC is automatically synchronized with the NTP time every few hours. The RTC has a time keeping accuracy of ±2ppm.

######Server 
The data is sent out to a server every second. The server address (or IP) and the URI as well as the API-key can be changed on the configuration page.

There is an option on the configuration page to send out all values instead of always sending the latest one only. On a slow connection this can lead to a data jam since all unsent data is first kept in RAM and then saved to the SD card (if available, data is discarded otherwise). The Mutatio tries to sendout the oldest value first and works its way to the latest value as fast as possible. **NOTE: This mode is currently not supported by the defluxiod server software.**

In normal sending mode only the latest value is sent. If the server connection fails, the data is lost (but is still logged to the SD card if available). If the server connection always fails, check if the server address is set correctly. To see the data sent to the server and the server's response on the serial monitor, uncomment these lines in sendtoserver.h:
```
 // Serial.print(header);
 // Serial.print(jsonstring);
 
 // Serial.println(line); //uncomment this line to see servers response on serial
```
