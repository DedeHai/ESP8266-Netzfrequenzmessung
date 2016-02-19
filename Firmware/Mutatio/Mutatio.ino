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
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <SD.h>
#include <Wire.h> //I2C for RTC
#include "RtcDS3231.h" //RTC library: https://github.com/Makuna/Rtc
#include <EEPROM.h>
//#include <math.h>
#include "global.h"
#include "eepromhandling.h"
#include "SDcard.h"
#include "RTC_functions.h"
#include "LED.h"
#include "NTP_Time.h"
#include "thingspeak.h"
#include "interrupt.h"
#include "plotly.h"
#include "OTA_update.h"
#include "sendtoserver.h"
#include "webpage.h"
/*
   TODO:
   -find reason for memory leak in server send function (seems to have disappeared, may come back again)
*/



const double Filterconstant = 0.6; //constant for lowpass filter
double lowpassvalue = 0; //lowpass filtered frequency offset
//todo: put the clockoffset value in eeprom


void setup() {

  delay(200); //wait for power to stabilize
  Serial.begin(115200);
  memset(measurementdata, 0, sizeof(measurementdata));
  Serial.println(F("\r\nMutatio grid frequency tracker, part of netzsin.us project"));

  EEPROM.begin(256); //eeprom emulation on flash, the ESP8266 has no actual EERPOM
  if (!ReadConfig())
  {
    writeDefaultConfig();
  }

  pinMode(PIXEL_PIN, OUTPUT);
  LED.begin();
  LEDcolor.r = 100;
  LEDcolor.g = 5;
  LEDcolor.b = 1;
  LED.setPixelColor(0, LED.Color(LEDcolor.r, LEDcolor.g, LEDcolor.b));
  LED.show(); // Initialize LED color

  if (config.useRTC)
  {
    RTCinit();
  }

  wifiWatchdog = 1; //wifi not connected
  sdWatchdog = 1; //SD not initialized
  signalWatchdog = 1000; //signal not initilized

  Serial.print("connecting to ");
  Serial.println(config.ssid);
  WiFi.disconnect(true); //delete any old wifi configuration
  WiFi.mode(WIFI_STA);
  //  delay(100);
  WiFi.begin(config.ssid.c_str(), config.password.c_str());
  if (!config.useDHCP)
  {
    WiFi.config(IPAddress(config.IP[0], config.IP[1], config.IP[2], config.IP[3] ),  IPAddress(config.Gateway[0], config.Gateway[1], config.Gateway[2], config.Gateway[3] ) , IPAddress(config.Netmask[0], config.Netmask[1], config.Netmask[2], config.Netmask[3] ));
  }

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  if (config.useSDcard)
  {
    SDinit(SD_CSN_PIN);
    SDwriteLogfile("Boot");
  }


  server.on ( "/", sendPage); //send the config page
  server.onNotFound(handleNotFound);//handle page not found
  server.begin(); //start webserver


  pinMode(MEASUREMENTPIN, INPUT); //is the default, just making sure
  attachInterrupt(MEASUREMENTPIN, pininterrupt, CHANGE); //trigger both rising and falling


 


  config.useDHCP = true;
}


//at 50.000Hz, the number of 80MHz ticks is 1600000
void loop() {

  uint8_t SDreadycounter = 0; //counts measurements ready to be written to SD card (i.e. they were already printed to Serial)
  checkButtonState(); //check state of 'FLASH' button, stat accesspoint if pressed for some time
  updateLED();


  //check if there is old data avialable on the SD card and send it to server
  if (WiFi.status() == WL_CONNECTED)
  {
    if (wifiWatchdog > 0) //wifi was not connected before, print some info, init some wifi related stuff
    {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      wifiWatchdog = 0; //connected to wifi
      timeManager(1); //force time update from NTP server
      initOTAupdate();
      SDwriteLogfile("WIFI connected");

      plotly_init(false); //!!! comment this line if not using plotly server
      
    }
    ArduinoOTA.handle();

    if (unsentSDData) //unsentSDData is only set if config.sendAllData is enabled
    {
      sendSDdataToServer();
    }
  }
  else
  {
    if (wifiWatchdog == 0) //wifi was connected before
    {
      SDwriteLogfile("WIFI disconnected");
      WiFi.begin(config.ssid.c_str(), config.password.c_str());
      if (!config.useDHCP)
      {
        WiFi.config(IPAddress(config.IP[0], config.IP[1], config.IP[2], config.IP[3] ),  IPAddress(config.Gateway[0], config.Gateway[1], config.Gateway[2], config.Gateway[3] ) , IPAddress(config.Netmask[0], config.Netmask[1], config.Netmask[2], config.Netmask[3] ));
      }
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
    if (wifiWatchdog > 10000) wifiWatchdog = 0; //try to reconnect to WiFi every ~10 seconds
    delay(1);
  }

  if (issampling == false) ////wait for interrupt to capture a measurement and write data to the buffer
  {
    uint16_t i = datareadindex;
    uint16_t j;
    uint8_t thingspeakFailed = 0;
    uint8_t serverFailed = 0;
    uint16_t latestindex = datareadindex;

    for (j = 0; j < MEASUREMENTVALUES; j++)
    {
      if (measurementdata[i].flag > 0 && ((measurementdata[i].flag & 0x02) == 0)) //not yet printed
      {
        latestindex = i;
        measurementdata[i].flag |= 0x02; //data printed, can now be saved to SD
        printMeasurement(i);
        
        if(sdWatchdog >0) //not using SD or SD failed 
        {
          datareadindex = i; //update the read index (is usually done in SD function after saving the value)
        }

        //make LED color green if 50.0Hz, red if too high, blue if too low
        int16_t colorindicator = measurementdata[i].data / 70;
        if (colorindicator > 85) colorindicator = 85;
        else if (colorindicator < -85) colorindicator = -85;
        LEDcolor = hsv_to_rgb(85 - colorindicator, 255, 20);

        if (config.sendAllData)
        {
          if (unsentSDData == 0 && measurementdata[i].flag > 0 && (measurementdata[i].flag & 0x08) == 0 && WiFi.status() == WL_CONNECTED && serverFailed == 0) //not yet sent and we have a wifi connection
          {
            if (sendMeasurementToServer(measurementdata[i]) == 0) //send successful?
            {
              measurementdata[latestindex].flag |= 0x08; //mark as sent out
            }
            else
            {
              serverFailed = 1; //try again next time
            }
          }
        }


        //thingspeak sendout & heap display for debugging
        /*
                if ((millis() - thingspeakTime > 15000) && (thingspeakFailed == 0) && WiFi.status() == WL_CONNECTED) //send this one out to thingspeak
                {
                  // thingspeakFailed = updateThingspeak(measurementdata[i].data);
                  Serial.print("Free heap:");
                  Serial.println(ESP.getFreeHeap(), DEC); //debug: check for memory leaks
                  thingspeakTime = millis();
                  SDwriteLogfile("Test");
                }
        */



      }
      else if (measurementdata[i].flag > 1 && ((measurementdata[i].flag & 0x04) == 0) ) SDreadycounter++; //count number of measurements ready to be written to SD



      i++;
      if (i >= (MEASUREMENTVALUES)) i = 0; //continue at beginning if end of buffer reached
    }

    //send only the latest measurement (put in above for loop to send them all, change latestindex to i
    if (measurementdata[latestindex].flag > 0 && (measurementdata[latestindex].flag & 0x08) == 0 && WiFi.status() == WL_CONNECTED && serverFailed == 0 && !config.sendAllData) //not yet sent and we have a wifi connection
    {
      plotly_plot(measurementdata[latestindex]);
      /*
        if (sendMeasurementToServer(measurementdata[latestindex]) == 0) //send successful?
        {
        measurementdata[latestindex].flag |= 0x08; //mark as sent out
        }*/
    }

    issampling = true;
  }


  if (config.useSDcard)
  {
    if (SD_initialized) //card is initialized //note: tried to add analogRead here, it makes the wifi disconnect very frequently, seems to be a known issue
    {
      if ((MEASUREMENTVALUES - SDreadycounter) < 70) //buffer is getting full, better save it to SD
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
    else if (config.useSDcard && sdWatchdog == 1)
    {
      SDinit(SD_CSN_PIN);
    }
    else sdWatchdog++;
  }
  timeManager(0);
  yield();
  server.handleClient();

  yield();
  signalWatchdog++;
}



