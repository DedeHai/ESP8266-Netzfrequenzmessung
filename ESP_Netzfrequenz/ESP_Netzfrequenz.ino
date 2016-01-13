

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <math.h>
#include "global.h"
#include "NTP_Time.h"
#include "thingspeak.h"
/*
   TODO: if NTP fails several times, get a new address
   Also: if write-measurement value fails, get a new NTP value from the RTC or something
*/
//#include "pingfunctions.h"


volatile uint8_t toggle;
volatile uint32_t timekeeper = 0;
volatile uint8_t interruptnoisedetector = 0;
volatile uint32_t capturetime[NUMBEROFCAPTURES];
volatile uint32_t lastcapture = 0;
volatile uint8_t arrayindex = 0;
volatile boolean issampling = true;
volatile boolean firsttrigger = true; //skip the first captures after startup, it is always wrong
const double Filterconstant = 0.6; //constant for lowpass filter
double lowpassvalue = 0; //lowpass filtered frequency offset
//todo: put the clockoffset value in eeprom

const char* ssid = "IUY-12651_EXT";
const char* password = "Dinosaurier-sind-Vegetarier";



void ICACHE_RAM_ATTR pininterrupt() {

  float frequencyoffset;
  int32_t noise;
  timekeeper = ESP.getCycleCount();

  if (timekeeper > lastcapture) //check 32bit integer overflow integer overflow
  {
    capturetime[arrayindex] = ESP.getCycleCount() - lastcapture; //calculate the ticks since last interrupt
  }
  else
  {
    capturetime[arrayindex] = lastcapture - ESP.getCycleCount(); //calculate the ticks since last interrupt
  }

  //check for valid time: must be 160000±10%
  noise = ((int32_t)capturetime[arrayindex] - (int32_t)1600000);
  if (noise < 16000 || noise > -16000)
  {
    arrayindex++; //noise ok, keep this value
  }

  if (arrayindex >= NUMBEROFCAPTURES)
  {
    uint16_t i;
    uint8_t baddata = 0;
    int32_t noise;
    uint32_t ticks = 0;

    //double check for noise, keep the tolerance tighter than before
    for (i = 0; i < NUMBEROFCAPTURES; i++)
    {

      noise = ((int32_t)capturetime[i] - (int32_t)1600000);
      if (noise > 12000 || noise < -12000)
      {
        baddata++; //too much noise
      }
      else
      {
        ticks += capturetime[i];
      }
    }

    if (baddata < 3) //only allow a few bad datapoints
    {
      frequencyoffset = ((((float)FCPU + FCPUerror) / ((float)ticks / (NUMBEROFCAPTURES - baddata))) - 50.0) * 10000; //frequency offset to 50.000Hz in [mHz]*10
      writeMeasurement((int16_t)frequencyoffset);
    }

    arrayindex = 0;
    issampling = false;
  }

  lastcapture = timekeeper; //save the captured time for next time
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("\r\nNetzfrequenzmesser V3");
  pinMode(4, INPUT_PULLUP);

  memset(measurementdata, 0, sizeof(measurementdata));
  /*
    EEPROM.begin(128);
    if (!ReadConfig())
    {
      writeDefaultConfig();
    }*/
  config.APIkey = "NQLMU4MBQ98TZ425";

  Serial.print("connecting to ");
  Serial.print(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  updatelocaltime = true;
  attachInterrupt(4, pininterrupt, RISING);
  timeManager();
}


//at 50.000Hz, the number of 80MHz ticks is 1600000
void loop() {


  static uint8_t toggle = 0;
  static uint32_t updateClock = 0;
  int i;

  if (issampling == false) ////wait for interrupt to capture the values and write data to the buffer
  {

    //lowpassvalue = ((double)frequencyoffset *  Filterconstant) + (lowpassvalue  * ((double)1.0 - Filterconstant));
    for (i = 0; i < MEASUREMENTVALUES; i++)
    {
      if (measurementdata[i].flag == 1)
      {
        measurementdata[i].flag = 0;
        printMeasurement(i);

        if (millis() - thingspeakTime > 15000) //send this one out to thingspeak
        {
          //detachInterrupt(4);
          updateThingspeak(measurementdata[i].data);
          //attachInterrupt(4, pininterrupt, RISING);
          Serial.print("Free heap:");
          Serial.println(ESP.getFreeHeap(), DEC); //debug: check for memory leaks
        }
      }
    }

    issampling = true;
  }

  timeManager();



  yield();
}



