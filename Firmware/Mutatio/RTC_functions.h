

uint8_t updatelocaltimefromRTC(void)
{
   Serial.println(F("Updating from RTC"));
  RtcDateTime RTCtime  = RTC.GetDateTime();
  RtcDateTime RTCtemp  = RTC.GetDateTime();
  if (RTCtime.Epoch32Time() > 1452874314) //check if RTC could be valid
  {
    uint16_t timeout = 0;
    //wait for the RTC time to roll over by one second:
    while (RTCtime.Second() == RTCtemp.Second() && timeout < 1000)
    {
      delay(1);
      RTCtime  = RTC.GetDateTime();
      timeout++;
    }

    if (timeout >= 1000) return -1;

    //convert to NTP time:
    uint32_t NTPtime = RTCtime.Epoch32Time() + 2208988800UL;
    //now set the local time:
    localTime.NTPtime = NTPtime;
    localTime.milliseconds = 0;
    localTime.millistimestamp = millis();
    localTimeValid = true;
  }
  else
  {
    return -1;
  }
  Serial.println(F("Local time set from RTC"));
  return 0;
}

void updateRTCfromlocaltime(void)
{
   Serial.println(F("Synchronizing RTC with NTP"));
  RtcDateTime RTCtime  = RTC.GetDateTime();
  RtcDateTime RTCtemp  = RTC.GetDateTime();

  uint16_t timeout = 0;
  //wait for the RTC time to roll over by one second:
  while (RTCtime.Second() == RTCtemp.Second() && timeout < 1000 )
  {
    delay(1);
    RTCtime  = RTC.GetDateTime();
    timeout++;
  }

  if (timeout >= 1000) return;

  RTC.SetIsRunning(false); //stop the RTC
  //wait for the local second to roll over:
  while ((millis() - localTime.millistimestamp + localTime.milliseconds) % 1000 < 999)
  {
    yield();
  }


  uint32_t epoch = (localTime.NTPtime - 2208988800UL) + ((millis() - localTime.millistimestamp) + localTime.milliseconds + 2) / 1000;
  //if (((millis() - localTime.millistimestamp + localTime.milliseconds) % 1000) >= 500) epoch++; //round up if needed

  RTCtime.InitWithEpoch32Time(epoch);
  RTC.SetDateTime(RTCtime);

  RTC.SetIsRunning(true); //start the RTC
  Serial.print(F("RTC set: "));
  Serial.print(RTCtime.Year());
  Serial.print(" ");
  Serial.print(RTCtime.Month());
  Serial.print(" ");
  Serial.print(RTCtime.Day());
  Serial.print(" ");
  Serial.print(RTCtime.Hour());
  Serial.print(".");
  Serial.print(RTCtime.Minute());
  Serial.print(":");
  Serial.println(RTCtime.Second());

  RTCTimeValid = true;

}


void RTCinit(void)
{
   Serial.println(F("RTC init"));
  Wire.begin(); //default pins: 4 & 5
  RTC.Begin();
  RTC.Enable32kHzPin(false);
  RTC.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  RTC.SetIsRunning(true); //start the RTC in case it was not running (prevents infinite loop)
  RtcDateTime RTCtime  = RTC.GetDateTime();
  if (RTCtime.Epoch32Time() > 1452874314) //check if RTC could be valid
  {
    RTCTimeValid = true;
  }
}


