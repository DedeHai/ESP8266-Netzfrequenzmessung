//if CPU clock is running too slow (i.e. lower than 80MHz) the internal time will also run slow and the time offset to the NTP server will become negative
#define FCPU 80000000UL //CPU clock frequency (will be corrected with clock offset in calculation)
#define NUMBEROFCAPTURES 50 //number of values to capture from frequency input
#define MEASUREMENTVALUES 256 //array with this many values is created

struct Config {
  String ssid; //32 bytes maximum
  String password; //32 bytes maximum
  String APIkey;
  String DeviceName;  //32 bytes maximum, name for access point
  String DevicePW;  //32 bytes maximum, password for access point
  String ntpServerName;  //32 bytes maximum
  uint8_t IP[4];
  uint8_t Netmask[4];
  uint8_t Gateway[4];
  int16_t FCPUerror;
} config;


struct Measurement {
  uint32_t Timestamp;
  int16_t milliseconds; //millisecond offset of measurement to timestamp
  int16_t data;
  uint8_t flag; //flag valid data (for displaying and sending out)
};

struct timeStruct {
  uint32_t NTPtime; //seconds of time since 1900
  int16_t milliseconds; //fraction of seconds of time in milliseconds (can become negative! but only -5 milliseconds max)
  uint32_t millistimestamp; //millis() time when the time was last synchronized
};

int FCPUerror = -175; //error in CPU clock =  (offset in [ms]) * FCPU / (time in [ms] over which offset was measured)  -> clockoffset is negative if time offset is negative

timeStruct localTime; //the global local time variables holding current NTP time
bool localTimeValid = false; //is true once the local time contains valid time data (to prevent corrupted measurement data)
unsigned long thingspeakTime = 0; //used in main loop to control when data is sent out (must not do it faster than every 15 seconds)


Measurement measurementdata[128];

void getNowTime(timeStruct* t)
{
  uint64_t nowmilliseconds = (uint64_t)localTime.NTPtime * 1000 + localTime.milliseconds + ((4294967295UL - localTime.millistimestamp) + millis());
  t->NTPtime = nowmilliseconds / 1000;
  t->milliseconds =  nowmilliseconds - (uint64_t)t->NTPtime * 1000;
  t->millistimestamp = millis();
}

int8_t ICACHE_RAM_ATTR writeMeasurement(int16_t value)
{
  static int measurementindex = 0;
  if (localTimeValid) //can only write the value if local time is set
  {
    timeStruct nowTime;
    getNowTime(&nowTime);
    measurementdata[measurementindex].data = value;
    measurementdata[measurementindex].Timestamp = nowTime.NTPtime;
    measurementdata[measurementindex].milliseconds = nowTime.milliseconds - (NUMBEROFCAPTURES * 20) / 2;
    if ( measurementdata[measurementindex].milliseconds < 0)
    {
      measurementdata[measurementindex].Timestamp--;
      measurementdata[measurementindex].milliseconds += 1000;
    }
    else if (measurementdata[measurementindex].milliseconds > 1000)
    {
      measurementdata[measurementindex].Timestamp++;
      measurementdata[measurementindex].milliseconds -= 1000;
    }
    measurementdata[measurementindex].flag = 1;
    measurementindex++;
    if (measurementindex >= MEASUREMENTVALUES) measurementindex = 0;
    return 0;
  }
else return -1;
}

void printMeasurement(int i)
{
  Serial.print((float)measurementdata[i].data/10);
  Serial.print("\t@GMT ");

  unsigned long epoch = measurementdata[i].Timestamp - 2208988800UL;

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
  Serial.println(measurementdata[i].milliseconds);


}

