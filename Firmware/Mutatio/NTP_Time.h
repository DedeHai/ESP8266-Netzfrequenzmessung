
#define REQUESTSTOAVERAGE 50 //number of requests to send to server to get the average time during fastupdate
#define ALLOWEDROUNDTRIPDELAY 30 //maximum allowed ping for the NTP server (default: 30) Absolute time accuracy depends on this value
//#define USE_RTC_FOR_CPUSYNC 1 //uncomment this define to use RTC instead of NTP to sync FCPU, use if NTP sync is inaccurate (i.e. more than 2ms fluctuation)

//ntp stuff
IPAddress timeServerIP; // pool.ntp.org NTP server address
const char* ntpServerName = "pool.ntp.org";//"time.nist.gov";//"north-america.pool.ntp.org"
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP
const unsigned int UDPlocalPort = 2390;


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address, uint32_t* timestamp)
{
  //Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  *timestamp = millis();
}

void printUTCtime(uint32_t NTPTime)
{
  // now convert NTP time into everyday time:
  // Serial.print("Unix time = ");
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  // subtract seventy years:
  unsigned long epoch = NTPTime - 2208988800UL;
  // print Unix time:
  // Serial.println(epoch); //unix time

  // print the hour, minute and second:
  Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
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
  Serial.println(epoch % 60); // print the second

}


//synchronize the local clock with the NTP server's
//return value is the time server roundtrip delay (below 20ms yields pretty good timestamps)
int NTP_gettime(timeStruct* t)
{
  Serial.print(F("NTP Sync "));
  uint32_t NTP_start_time; //local timestamp to measure network roundtrip time
  uint32_t timeout = millis();
  uint8_t errorcounter = 0;
  int gotresponse = 0;

  // Serial.println("Starting UDP");
  udp.begin(UDPlocalPort);
  // Serial.print("Local port: ");
  // Serial.println(udp.localPort());

  WiFi.hostByName(ntpServerName, timeServerIP); //get ip for timeserver to use
  timeout = millis();
  sendNTPpacket(timeServerIP, &NTP_start_time); // send an NTP packet to a time server
  // wait to see if a reply is available
  do {
    yield();
    gotresponse = udp.parsePacket();
  } while (!gotresponse && (millis() - timeout < 300));


  if (gotresponse == NTP_PACKET_SIZE) {

    t->millistimestamp = millis(); //save the current millis() time at reception of packet
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    udp.stop();
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

    if ((highWord << 16 | lowWord) == 0)
    {
      Serial.println(F(" FAIL1"));
      NTPfailcounter++;
      return -999;
    }
    //Serial.println(highWord << 16 | lowWord);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    t->NTPtime = highWord << 16 | lowWord;
    //extract the fraction of the second:
    t->milliseconds =  ((uint32_t)((packetBuffer[44] << 8) + packetBuffer[45]) * 1000 / 65536); //fraction in milliseconds
    //add half of the roundtrip delay time:
    t->milliseconds =  t->milliseconds + (t->millistimestamp - NTP_start_time) / 2; //is rounded off but its not that accurate anyways
    //check if this got us to the next full second:
    if ( t->milliseconds >= 1000)
    {
      t->NTPtime += 1;
      t->milliseconds -= 1000;
    }
    Serial.println(F(" OK"));

  }
  else {
    udp.stop();
    Serial.println(F(" FAIL2"));
    NTPfailcounter++;
    return -999;
  }

  return t->millistimestamp - NTP_start_time;
}



//function returns the offset (in ms) of the local time to the time struct given in the argument
int getLocalTimeOffset(timeStruct* t)
{
  uint64_t nowmilliseconds = (uint64_t)localTime.NTPtime * 1000 + localTime.milliseconds + (millis() - localTime.millistimestamp);
  uint64_t NTPmilliseconds = (uint64_t)t->NTPtime * 1000 + t->milliseconds + (millis() - t->millistimestamp);
  int32_t millisdifference = int32_t(nowmilliseconds - NTPmilliseconds);

  return millisdifference;
}

void syncFCPU(void)
{
  /*
     es muss die annahme gelten, dass der NTC sync genau ist. ansonsten hat man keine solide zeitbasis.
     für ausnahmefälle kann man auch den RTC zur synchronisation nehmen, das sollte aber nur per firmware
     update möglich sein. am besten mit einem #define und dann hier abfragen.
     fürs NTP update: ein timestamp der localtime nehmen, diese wird mit dem NTP permanent abgeglichen
     indem der offset beim getNowTime schon abgezogen wird.
     um zu prüfen:
     schauen ob millis einen overflow hatte
     dann entsprechend vom timestamp die zeit über millis berechnen
     abweichung von getNowTime berechnen und daraus FCPU error ausrechnen

     beim RTC:
     dasselbe, aber mit der RTC time.


  */
  //synchronize FCPU with NTP time (or RTC)

  static uint8_t syncinprogress = 0;
  static timeStruct FCPU_timestamp; //time struct of localtime
  static uint32_t synctime;
  if (syncinprogress == 0)
  {
    //start new sync, save current time:
    Serial.println(F("FCPU timesync started"));
#ifdef USE_RTC_FOR_CPUSYNC //use RTC
    RtcDateTime RTCtime  = RTC.GetDateTime();
    RtcDateTime RTCtemp  = RTC.GetDateTime();
    uint16_t timeout = 0;
    //wait for the RTC time to roll over by one second:
    while (RTCtime.Second() == RTCtemp.Second() && timeout < 1000)
    {
      delay(1);
      RTCtime  = RTC.GetDateTime();
      timeout++;
    }
    if (timeout >= 1000) return;

    //convert to NTP time:
    uint32_t NTPtime = RTCtime.Epoch32Time() + 2208988800UL;
    //now set the timestamp:
    FCPU_timestamp.NTPtime = NTPtime;
    FCPU_timestamp.milliseconds = 0;
    FCPU_timestamp.millistimestamp = millis();
#else //use NTP 
    getNowTime(&FCPU_timestamp);
#endif
    syncinprogress = 1;
    synctime = millis();
    RTCsynctime++;
  }
  else
  {
    if (millis() - synctime > 1800000) //check FCPU sync after 30 minutes
    {
      if (FCPU_timestamp.millistimestamp < millis()) //check if millis overflowed, start a new sync if it did (timestamps are not valid)
      {
        //check deviation of millis() counter to absolute time
        timeStruct nowtimestamp;
#ifdef USE_RTC_FOR_CPUSYNC //use RTC

        RtcDateTime RTCtime  = RTC.GetDateTime();
        RtcDateTime RTCtemp  = RTC.GetDateTime();
        uint16_t timeout = 0;
        //wait for the RTC time to roll over by one second:
        while (RTCtime.Second() == RTCtemp.Second() && timeout < 1000)
        {
          delay(1);
          RTCtime  = RTC.GetDateTime();
          timeout++;
        }
        if (timeout >= 1000) return;

        //convert to NTP time:
        uint32_t NTPtime = RTCtime.Epoch32Time() + 2208988800UL;
        //now set the timestamp:
        nowtimestamp.NTPtime = NTPtime;
        nowtimestamp.milliseconds = 0;
        nowtimestamp.millistimestamp = millis();
#else //use NTP 
        getNowTime(&nowtimestamp);
#endif

        //we now have two timestamps, current time and time at last sync start
        //calculate the cpu now time again from millis and sync timestamp and check
        //deviation from real time
        uint64_t nowmilliseconds = (uint64_t)nowtimestamp.NTPtime * 1000 + nowtimestamp.milliseconds + (millis() - nowtimestamp.millistimestamp);
        uint64_t syncmilliseconds = (uint64_t)FCPU_timestamp.NTPtime * 1000 + FCPU_timestamp.milliseconds + (millis() - FCPU_timestamp.millistimestamp);
        int32_t timedifference = syncmilliseconds - nowmilliseconds;
        if (timedifference < 1000 && timedifference > -1000) //if time deviation is more than a second, something went wrong (usually is a few milliseconds)
        {
          int32_t newFCPUerror = ((double)timedifference * FCPU) / ((nowtimestamp.NTPtime - FCPU_timestamp.NTPtime) * 1000); //  (offset in [ms]) * FCPU / (time in [ms] over which offset was measured)
          Serial.print(F("FCPU timesync finished. Timedeviation: "));
          Serial.print(timedifference);
          Serial.print(F("ms in a time period of "));
          Serial.print(nowtimestamp.NTPtime - FCPU_timestamp.NTPtime);
          Serial.println(F("s"));
          config.FCPUerror = newFCPUerror;
          //if error is much different from eeprom saved one, update it in eeprom (prevents flash wearing if not done too often)
          if (newFCPUerror - config.FCPUerror < 40 || newFCPUerror - config.FCPUerror  > -40)
          {
            WriteConfig(); //write back to flash ("eeprom")
          }
          String cpuerroroutput = "CPU frequency error is " + String(config.FCPUerror);
          Serial.println(cpuerroroutput);
          SDwriteLogfile(cpuerroroutput); //log to sd card
        }
        else
        {
          Serial.print("something went wrong in FCPU time keeping"); // !!!debug
          Serial.println(timedifference); //!!!debug
        }
        syncinprogress = 0; //done, start a new sync
      }
      else syncinprogress = 0; //error, start a new sync
    }
  }

  if (RTCsynctime > 10) //update RTC from localtime every few hours
  {
    updateRTCfromlocaltime();
    RTCsynctime = 0;
  }


}

//call this function periodically, it manages time and clock calibration
//on bootup, the time will be set. this function reads the NTP time periodically (every few minutes) and
//corrects the local time when the offset becomes bigger than 10ms
//it records the time it takes to reach the 5ms offset and sets the clock offset for the frequency calculation
//the CPU clock itself cannot be changed but this way an accurate timekeeping is assured and the
//cpu clock is constantly monitored and corrected
//note: an offset of 1mHz in the measurement equals a CPU clock offset of 1ms per 50s. Usually the CPU clock is much more accurate.
//more like 1ms error in 5 minutes
void timeManager(uint8_t forceTimeSync)
{
  static uint32_t NTPupdate = 0; //timestamp on when to update from the NTP server
  static int fastupdate = REQUESTSTOAVERAGE; //update NTP quicker at startup to calibrate the time
  timeStruct temptime; //time struct to get data from servers
  bool getNTPupdate = false; //if set true, a timestamp is requested from NTP server and evaluated

  if (NTPupdate > 0 && millis() < NTPupdate) // millis() did overflow!
  {
    //deactivate the interrupt while we update the time:
    noInterrupts();
    NTPupdate = millis();
    //calculate a new NTP time from overflown millis()
    uint64_t nowmilliseconds = (uint64_t)localTime.NTPtime * 1000 + localTime.milliseconds + ((4294967295UL - localTime.millistimestamp) + millis());
    localTime.NTPtime = nowmilliseconds / 1000;
    localTime.milliseconds =  nowmilliseconds - (uint64_t)localTime.NTPtime * 1000;
    localTime.millistimestamp = millis();
    interrupts();
  }
  //run the rest of the code only if the WIFI is available:
  if (WiFi.status() == WL_CONNECTED) {

    if ((localTimeValid == false || forceTimeSync) && NTPfailcounter < 10)
    {
      uint8_t errorcounter = 0;
      int roundtripdelay;
      uint32_t timevalidation;
      fastupdate = REQUESTSTOAVERAGE;
      //update the local time immediately!
      do
      {
        roundtripdelay = NTP_gettime(&localTime); //update the local time
        delay(100);
        roundtripdelay += NTP_gettime(&temptime); //get another timestamp to validate
        if (localTime.NTPtime > 3661332195) //true if we got a valid timestamp
        {
          timevalidation = temptime.NTPtime - localTime.NTPtime; //should be zero or one if we got the same time twice
        }
        else timevalidation = 2; //time we got is invalid for sure.
        errorcounter++;
        Serial.println(NTPfailcounter);
      } while ((roundtripdelay <= 0 || roundtripdelay > ALLOWEDROUNDTRIPDELAY * 5) && timevalidation > 1 && errorcounter < 100 && NTPfailcounter < 10);


      if (errorcounter < 100 && NTPfailcounter < 10) {
        localTimeValid = true;
        localtimeoffset = 0;
        Serial.print(" Local Time Initialized: ");
        printUTCtime(localTime.NTPtime);
        //updateRTCfromlocaltime();
        NTPupdate = millis();
      }
      else if (NTPfailcounter >= 10)
      {
        //internet is not available, start the accesspoint to allow reconfiguration
        APactive = 1;
      }
      else if (RTCTimeValid == true)
      {
        if (updatelocaltimefromRTC() == 0)
        {
          localTimeValid = true;
        }
      }
      return; //do not run the code below if server access was already attempted
    }

    if (fastupdate > 0)
    {
      if (millis() - NTPupdate > 1000) //check time once every second during fastupdate
      {
        NTPupdate = millis();
        getNTPupdate = true;
      }
    }
    else
    {
      if (millis() - NTPupdate > 60000) //check time once per minute
      {
        NTPupdate = millis();
        getNTPupdate = true;
      }
      fastupdate = -1; //prevents underflow in decrement below
      syncFCPU();  //only synchronize FCPU if no fastupdate is going on
    }

    if (getNTPupdate)
    {
      int roundtripdelay = NTP_gettime(&temptime);
      //  Serial.println(roundtripdelay);
      if (roundtripdelay > 0 && roundtripdelay < ALLOWEDROUNDTRIPDELAY) //successfully got the time and it is of good quality
      {
        int newoffset = getLocalTimeOffset(&temptime);
        if (fastupdate < 0) //this is not a fastupdated, offset must be small, check if it is
        {
          int offsetdeviation = localtimeoffset - newoffset;
          if ( offsetdeviation > ALLOWEDROUNDTRIPDELAY / 3 || offsetdeviation < -ALLOWEDROUNDTRIPDELAY / 3) //10ms deviation is way too big for an accurate NTP reading (usually is within ±5ms)
          {
            newoffset = localtimeoffset; //ignore this value in the filter below
            NTPfailcounter += 10; //if this happens repeatedly, local clock is out of sync for some reason, trigger a sync error and do a fastupdate
            NTPupdate = millis() - 59800;//get another value soon:
          }
        }
        // Serial.println(newoffset);
        localtimeoffset = ((float)newoffset *  0.2) + (localtimeoffset  * ((float)1.0 - 0.2));
        Serial.print("Local Time offset [ms] = ");
        Serial.print(localtimeoffset, 2);
        Serial.println("\t(" + String(newoffset) + ")");
        fastupdate--;

        if (fastupdate == 0 || localtimeoffset > ALLOWEDROUNDTRIPDELAY / 3 || localtimeoffset < -ALLOWEDROUNDTRIPDELAY / 3) //the offset is high or fastupdate request, update local clock
        {

          //set the new found (filtered) timeoffset:
          if (localtimeoffset < 0) localtimeoffset -= 0.5; //round to nearest integer when converting to int (not really necessary here)
          else  localtimeoffset += 0.5;

          //deactivate the interrupt while we update the time:
          noInterrupts();
          localTime.milliseconds =  localTime.milliseconds - (int)localtimeoffset;
          if ( localTime.milliseconds < 0)
          {
            localTime.NTPtime--;
            localTime.milliseconds += 1000;
          }
          else if (localTime.milliseconds >= 1000)
          {
            localTime.NTPtime++;
            localTime.milliseconds -= 1000;
          }

          localtimeoffset = 0;
          interrupts(); //reactivate the interrupts
          Serial.println("Local Time adjusted");
          NTPfailcounter = 0;
        }
      }
      else //did not get any good time value, try again soon
      {
        NTPupdate = millis() - 55000;
        NTPfailcounter++;
        if (NTPfailcounter > 600)
        {
          fastupdate = REQUESTSTOAVERAGE; //getting a lot of failed requests, do a fast update to keep track of time and not fall out of pace
          NTPfailcounter = 0;
        }
      }
    }
  }
  else //no wifi available, use the RTC
  {
    if ((localTimeValid == false || forceTimeSync) && RTCTimeValid == true) //update the local time from RTC
    {
      if (updatelocaltimefromRTC() == 0)
      {
        localTimeValid = true;
      }
    }
#ifdef USE_RTC_FOR_CPUSYNC
    syncFCPU();
#endif
  }
  if (ESP.getCycleCount() - lastcapture < (230000)) //check that there is low chance of upcoming interrupt during the cyclecount millis update
  {
    setCyclecountmillis();
  }
}

