/*important note:
   All fucntions that are called from the interrupt (including the interrupt itself and functions the functions call)
   must be put in RAM by putting the attribute ICACHE_RAM_ATTR or the system can and will crash eventually!

*/

#include "Arduino.h"
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <SD.h>
#include <Wire.h> //I2C for RTC
#include "RtcDS3231.h" //RTC library: https://github.com/Makuna/Rtc
#include <math.h>
#include "global.h"
#include "RTC_functions.h"
#include "LED.h"
#include "NTP_Time.h"
#include "thingspeak.h"
#include "interrupt.h"
#include "plotly.h"
#include "OTA_update.h"
#include "sendtoserver.h"
#include "SDcard.h"
/*
   TODO:
   -add a configuration web page
   -add EEPROM support for config values

*/



const double Filterconstant = 0.6; //constant for lowpass filter
double lowpassvalue = 0; //lowpass filtered frequency offset
//todo: put the clockoffset value in eeprom

const char* ssid = "****";
const char* password = "****";


void setup() {


  Serial.begin(115200);
  memset(measurementdata, 0, sizeof(measurementdata));
  Serial.println("\r\nNetzfrequenzmesser V3");
  Serial.print("connecting to ");
  Serial.println(ssid);

  //   delay(10000);

  /*
    EEPROM.begin(256);
    if (!ReadConfig())
    {
      writeDefaultConfig();
    }*/
  config.APIkey = "NQLMU4MBQ98TZ425";
  config.useSDcard = false;
  config.useRTC = false;

  pinMode(PIXEL_PIN, OUTPUT);
  LED.begin();
  LEDcolor.r = 100;
  LEDcolor.g = 5;
  LEDcolor.b = 1;
  LED.setPixelColor(0, LED.Color(LEDcolor.r, LEDcolor.g, LEDcolor.b));
  LED.show(); // Initialize LED color

  RTCinit();


  wifiWatchdog = 1; //wifi not connected
  sdWatchdog = 1; //SD not initialized
  signalWatchdog = 250; //signal not initilized

  WiFi.mode(WIFI_STA);
  //  WiFi.disconnect();
  //  delay(100);
  WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }

  SDinit(SD_CSN_PIN);
  SDwriteLogfile("Boot");


  //  LEDcolor.r = 0;
  //  LEDcolor.g = 80;
  //  LEDcolor.b = 1;
  //  LED.setPixelColor(0, LED.Color(LEDcolor.r, LEDcolor.g, LEDcolor.b));
  //  LED.show(); // Initialize LED color

  pinMode(MEASUREMENTPIN, INPUT); //is the default, just making sure
  attachInterrupt(MEASUREMENTPIN, pininterrupt, CHANGE); //trigger both rising and falling


  // plotly_init();
  // plotly_openStream();


}


//at 50.000Hz, the number of 80MHz ticks is 1600000
void loop() {

  uint8_t SDreadycounter = 0; //counts measurements ready to be written to SD card (i.e. they were already printed to Serial)

  updateLED();

  
  //check if there is old data avialable on the SD card and send it to server
  if (WiFi.status() == WL_CONNECTED)
  {
    if (wifiWatchdog > 0) //wifi was not connected before, print some info, init some wifi related stuff
    {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      initOTAupdate();
      timeManager(1); //force time update from NTP server
      wifiWatchdog = 0; //connected to wifi
      SDwriteLogfile("WIFI connected");
    }
    ArduinoOTA.handle();

    if (unsentSDData)
    {
      sendSDdataToServer();
    }
  }
  else
  {
    if (wifiWatchdog == 0) //wifi was connected before
    {
      SDwriteLogfile("WIFI disconnected");
      WiFi.begin(ssid, password);
      String state;
      if (WiFi.status() == 0) state = "Idle";
      else if (WiFi.status() == 1) state = "NO SSID AVAILBLE";
      else if (WiFi.status() == 2) state = "SCAN COMPLETED";
      else if (WiFi.status() == 3) state = "CONNECTED";
      else if (WiFi.status() == 4) state = "CONNECT FAILED";
      else if (WiFi.status() == 5) state = "CONNECTION LOST";
      else if (WiFi.status() == 6) state = "DISCONNECTED";
      Serial.println("WIFI not available: " + state);
    }
    wifiWatchdog++; //disconnected from wifi
    delay(100);
  }

  if (issampling == false) ////wait for interrupt to capture a measurement and write data to the buffer
  {
    //LEDcolor.blue++;
    //LED.show(); // Initialize LED color


    uint16_t i = datareadindex;
    uint16_t j;
    uint8_t thingspeakFailed = 0;
    uint8_t serverFailed = 0;
    uint16_t latestindex = 0;
    //lowpassvalue = ((double)frequencyoffset *  Filterconstant) + (lowpassvalue  * ((double)1.0 - Filterconstant));
    for (j = datareadindex; j < datareadindex + MEASUREMENTVALUES; j++)
    {
      if (measurementdata[i].flag > 0 && (measurementdata[i].flag & 0x02) == 0) //not yet printed
      {
        latestindex = i;
        measurementdata[i].flag |= 0x02; //data printed, can now be saved to SD
        printMeasurement(i);

        // plotly_plot(measurementdata[i]);

        //make LED color green if 50.0Hz, red if too high, blue if too low
        int16_t colorindicator = measurementdata[i].data / 70;
        if (colorindicator > 85) colorindicator = 85;
        else if (colorindicator < -85) colorindicator = -85;
        LEDcolor = hsv_to_rgb(85 - colorindicator, 255, 50);

        if ((millis() - thingspeakTime > 15000) && (thingspeakFailed == 0) && WiFi.status() == WL_CONNECTED) //send this one out to thingspeak
        {
          thingspeakFailed = updateThingspeak(measurementdata[i].data);
          Serial.print("Free heap:");
          Serial.println(ESP.getFreeHeap(), DEC); //debug: check for memory leaks

          // Serial.print(millis());
          // Serial.print(" ");
          //Serial.println(getMillisfromCycleCount());
        }


      }
      else if (measurementdata[i].flag > 1 && ((measurementdata[i].flag & 0x04) == 0) ) SDreadycounter++; //count number of measurements ready to be written to SD



      i++;
      if (i >= (MEASUREMENTVALUES)) i = 0; //continue at beginning if end of buffer reached
    }

    //send only the latest measurement (put in above for loop to send them all, change latestindex to i
    if (unsentSDData == 0 && measurementdata[latestindex].flag > 0 && (measurementdata[latestindex].flag & 0x08) == 0 && WiFi.status() == WL_CONNECTED && serverFailed == 0) //not yet sent and we have a wifi connection
    {

      if (sendMeasurementToServer(measurementdata[latestindex]) == 0) //send successful?
      {
        measurementdata[latestindex].flag |= 0x08; //mark as sent out
      }
      else
      {
        serverFailed = 1; //try again next time
      }
    }

    issampling = true;
  }

  timeManager(0);

  if (SD_initialized) //card is initialized //note: tried to add analogRead here, it makes the wifi disconnect very frequently, seems to be a known issue
  {
    if ((MEASUREMENTVALUES - SDreadycounter) < 30) //buffer is getting full, better save it to SD
    {
      if (SDwriteMeasurements(SDreadycounter) != 0) //write failed, re-initialize the SD card
      {
        if (SDinit(SD_CSN_PIN) != 0)
        {
          yield();
          Serial.println(F("PANIC, NO SD CARD AVAILABLE!"));
          sdWatchdog = 1;
          //todo: inform the user by email, sms or fax
        }
      }
    }
  }
  else if (config.useSDcard)
  {
    SDinit(SD_CSN_PIN);
    delay(200);
  }

  signalWatchdog++;

  yield();
}



