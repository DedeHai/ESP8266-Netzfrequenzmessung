

//if CPU clock is running too slow (i.e. lower than 80MHz) the internal time will also run slow and the time offset to the NTP server will become negative
#define FCPU 80000000UL //CPU clock frequency (will be corrected with clock offset in calculation)
#define NUMBEROFCAPTURES 100 //number of values to capture from frequency pin input before generating a measurement (100 per second)
#define MEASUREMENTVALUES 120 //array with this many values is created (255 max!)
#define MEASUREMENTPIN 15 //pin which is used as clock input
#define PIXEL_PIN 0
#define ACCESSPOINTIMEOUT 300000 //timeout for accesspoint mode


RtcDS3231 RTC; //DS3231 RTC clock on I2C
ESP8266WebServer server(80); // The Webserver
WiFiClient client;
Adafruit_NeoPixel LED = Adafruit_NeoPixel(1, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void timeManager(uint8_t forceTimeSync); //function prototype

struct Config {
  String ssid; //32 bytes maximum
  String password; //32 bytes maximum
  String APIkey;
  String DeviceName;  //32 bytes maximum, name for access point
  String DevicePW;  //32 bytes maximum, password for access point
  uint8_t IP[4];
  uint8_t Netmask[4];
  uint8_t Gateway[4];
  bool useDHCP;
  bool useSDcard; //make use of the SD card
  bool useRTC; //make use of the RTC
  bool sendAllData; //send all data to server, not only the latest value, also sends SD backed up data if useSDcard is set
  int16_t FCPUerror;//error in CPU clock =  (offset in [ms]) * FCPU / (time in [ms] over which offset was measured)  -> clockoffset is negative if time offset is negative
  String serveraddress; //address string (or ip string) of server to send the data to
  uint16_t serverport;
  String serverURI; //URI of the server to stream to, for example "/api/submit/meter1" 31 chars maximum
  
} config;


struct Measurement {
  uint32_t Timestamp;
  int16_t milliseconds; //millisecond offset of measurement to timestamp
  int32_t data;
  uint8_t quality; //number of good datapoints used in this measurement value
  uint8_t flag; //flag showing status of the data, binary encoded: first bit is valid data, second bit is serial printed, third bit is written to SD, fourth bit is sent out to server
  int32_t lowpassfilteredmeasurement; //!!! for debug
};

struct timeStruct {
  uint32_t NTPtime; //seconds of time since 1900
  int16_t milliseconds; //fraction of seconds of time in milliseconds (can become negative! but only -5 milliseconds max)
  uint32_t millistimestamp; //millis() time when the time was last synchronized
};


timeStruct localTime; //the global local time variables holding current NTP time
bool localTimeValid = false; //is true once the local time contains valid time data (to prevent corrupted measurement data)
bool RTCTimeValid = false; //is set true after setting the RTC
unsigned long thingspeakTime = 0; //used in main loop to control when data is sent out (must not do it faster than every 15 seconds)
unsigned long plotlyTime = 0; //used in main loop to control when data is sent out (must not do it faster than every 15 seconds)
uint8_t SD_initialized = 0;
uint8_t RTCsynctime = 0; //keep track of when to sync the RTC (once every few hours is enough)

uint16_t wifiWatchdog;
uint16_t sdWatchdog;
uint16_t signalWatchdog;

volatile uint8_t toggle;
volatile uint32_t timekeeper = 0;
volatile uint8_t interruptnoisedetector = 0;
volatile uint32_t capturetime[NUMBEROFCAPTURES];
volatile uint32_t lastcapture = 0;
volatile uint16_t datareadindex = 0; //index in data measurement array that has to be read next when writing to SD card or sending to server (managed in SD card writing)
volatile boolean issampling = true;
int measurementindex = 0;
uint32_t cyclecountMillistimestamp;
uint32_t cyclecountatTimestamp;
uint8_t unsentSDData = 0; //unsent data available on SD card?
String unsentFilename = "unsent.txt";
uint8_t APactive = 0;
float localtimeoffset = 0; //low pass filtered offset in [ms] of local time compared to NTP time
uint16_t NTPfailcounter = 0;

Measurement measurementdata[MEASUREMENTVALUES];

//function returns the same value as millis() but can be fully put in RAM for no-crash interrupt access
//the values need to be updated at least every 60 seconds
uint32_t ICACHE_RAM_ATTR getMillisfromCycleCount(void) //assumes that the timestamp is updated before the CycleCounter overflows more than once
{
  uint32_t microseconds = ((ESP.getCycleCount() - cyclecountatTimestamp) / (FCPU / 1000000)); //80 cpu ticks per microsecond @ 80MHz
  if ((microseconds % 1000) > 500) microseconds += 500;
  return cyclecountMillistimestamp + (microseconds / 1000);
}

void setCyclecountmillis(void)
{
  noInterrupts();
  cyclecountatTimestamp = ESP.getCycleCount();
  cyclecountMillistimestamp = millis();
  interrupts();
}

void ICACHE_RAM_ATTR getNowTime(timeStruct* t)
{
  uint32_t milliseconds = getMillisfromCycleCount();
  uint64_t nowmilliseconds = (uint64_t)localTime.NTPtime * 1000 + (localTime.milliseconds  - (int)localtimeoffset) + (milliseconds - localTime.millistimestamp);
  t->NTPtime = nowmilliseconds / 1000;
  t->milliseconds =  nowmilliseconds - (uint64_t)t->NTPtime * 1000;
  t->millistimestamp = milliseconds;
}

void ICACHE_RAM_ATTR writeMeasurement(int32_t value, uint8_t gooddatapoints,  int32_t lowpassvalue) //!!!lowpassvalue for debug
{

  if (localTimeValid) //can only write the value if local time is set
  {
    timeStruct nowTime;
    getNowTime(&nowTime);
    measurementdata[measurementindex].data = value;
    measurementdata[measurementindex].quality = gooddatapoints;
    measurementdata[measurementindex].Timestamp = nowTime.NTPtime;
    measurementdata[measurementindex].milliseconds = nowTime.milliseconds;
    if ( measurementdata[measurementindex].milliseconds < 0)
    {
      measurementdata[measurementindex].Timestamp--;
      measurementdata[measurementindex].milliseconds += 1000;
    }
    else if (measurementdata[measurementindex].milliseconds >= 1000)
    {
      measurementdata[measurementindex].Timestamp++;
      measurementdata[measurementindex].milliseconds -= 1000;
    }
    measurementdata[measurementindex].flag = 1;
    measurementdata[measurementindex].lowpassfilteredmeasurement = lowpassvalue; //!!!for debug
    measurementindex++;
    if (measurementindex >= MEASUREMENTVALUES) measurementindex = 0;
  }
}

void printMeasurement(int i)
{
  Serial.print((float)measurementdata[i].data / 100);
  Serial.print("\t");
  Serial.print((float) measurementdata[i].lowpassfilteredmeasurement / 100);
  Serial.print("\t");
  Serial.print(measurementdata[i].quality);
  Serial.print("\t@UTC ");

  uint32_t epoch = measurementdata[i].Timestamp - 2208988800UL;

  // print the hour, minute and second:
  Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
  Serial.print(':');
  if ( ((epoch % 3600) / 60) < 10 ) {
    // In the first 10 minutes of each hour, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
  Serial.print(':');
  if ( (epoch % 60) < 10 ) {
    // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.print(epoch % 60); // print the second
  Serial.print('.');
  if (measurementdata[i].milliseconds < 100)
    Serial.print('0');
  if (measurementdata[i].milliseconds < 10)
    Serial.print('0');
  Serial.print(measurementdata[i].milliseconds);
   Serial.print("\t");
    char milliseconds[5];
  sprintf(milliseconds, "%03u", measurementdata[i].milliseconds); //need a fixed length, easiest using sprintf
   Serial.println(String(epoch) + "." + String(milliseconds));
}

String getJsonFromMeasurement(Measurement datastruct)
{
  //create JSON string, send it to server
  //defluxiod Json example: {"Timestamp":"2016-01-29T11:33:22.954022564+01:00","Value":49.9999}

  String datapoint;
  datapoint = String(((float)datastruct.data / 100000) + 50, 5);
  char milliseconds[5];
  sprintf(milliseconds, "%03u", datastruct.milliseconds); //need a fixed length, easiest using sprintf
  uint32_t epoch  = datastruct.Timestamp - 2208988800UL;
  
  /*
    RtcDateTime timeelements;
    timeelements.InitWithEpoch32Time(epoch); //converte epoch to date, month etc.
    char temparr[5];
    sprintf(temparr, "%02u", timeelements.Month()); //need a fixed length, easiest using sprintf
    String monthstr = String(temparr);
    sprintf(temparr, "%02u", timeelements.Day()); //need a fixed length, easiest using sprintf
    String daystr = String(temparr);
    sprintf(temparr, "%02u", timeelements.Hour() + 1); //need a fixed length, easiest using sprintf
    String hourstr = String(temparr);
    sprintf(temparr, "%02u", timeelements.Minute()); //need a fixed length, easiest using sprintf
    String minutestr = String(temparr);
    sprintf(temparr, "%02u", timeelements.Second()); //need a fixed length, easiest using sprintf
    String secondstr = String(temparr);

    String timestamp = String(timeelements.Year()) + "-" +  monthstr + "-" +  daystr;
    timestamp += "T" +  hourstr + ":" +  minutestr + ":" +  secondstr + "." + String(milliseconds);
    timestamp += "+01:00";
  */
  String timestamp = String(epoch) + "." + String(milliseconds);

  String jsonstring = "{\"Timestamp\": ";
  jsonstring += timestamp;
  jsonstring += ",\"Value\": ";
  jsonstring += datapoint;
  // jsonstring += ",\"Quality\": ";
  // jsonstring += String(datastruct.quality); //how many good values went into this measurment
  jsonstring += "}";
  return jsonstring;
}


void   checkButtonState(void)
{

  static uint16_t buttonstatecounter = 0;
  static uint32_t APtime = 0;

  //note: button is connected to the LED

  //make the pin an input, read it and set it to an output again
  pinMode(0, INPUT); //the pullup resistor takes around 4µs to make the state go high (default output state is low)
  delayMicroseconds(4); //wait for the pin to go high if button is not pressed
  if (digitalRead(0) == LOW)
  {
    buttonstatecounter++;
  }
  else buttonstatecounter = 0;
  pinMode(0, OUTPUT);

 // Serial.println(buttonstatecounter);

  if (buttonstatecounter > 2000)
  {
    APtime = millis();
    if (APactive == 0)   APactive = 1;
  }

  if (APactive > 0)
  {
    if (APactive == 1)
    {
      //immediately update the LED color (may make one measurement value bad but thats ok)
      LED.setPixelColor(0, LED.Color(10, 190, 200));
      LED.show();

      APactive = 2;
      if (WiFi.status() == WL_CONNECTED)
      {
        WiFi.mode(WIFI_AP_STA); //run both services only if connecte, AP is instable if not connected
      }
      else
      {
        WiFi.mode(WIFI_AP); //accesspoint is very unstable if STA mode is on and no wifi connection is available
      }
      WiFi.softAP(config.DeviceName.c_str(), config.DevicePW.c_str());
    }

    if (millis() - APtime > ACCESSPOINTIMEOUT)
    {
      WiFi.mode(WIFI_STA); //deactivate access point
      APactive = 0;
    }
  }



}

