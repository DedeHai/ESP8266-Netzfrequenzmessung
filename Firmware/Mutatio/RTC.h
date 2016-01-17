

uint8_t updatelocaltimefromRTC(void)
{
  RtcDateTime RTCtime  = RTC.GetDateTime();
  RtcDateTime RTCtemp  = RTC.GetDateTime();
 
  if (RTCtime.Epoch32Time() > 1452874314) //check if RTC could be valid
  {
    //wait for the RTC time to roll over by one second:
    while (RTCtime.Second() == RTCtemp.Second() )
    {
      delay(1);
      RTCtime  = RTC.GetDateTime();
    }
    //convert to NTP time:
    uint32_t NTPtime = RTCtime.Epoch32Time() + 2208988800UL;
    //now set the local time:
    localTime.NTPtime = NTPtime;
    localTime.milliseconds = 0;
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
  RtcDateTime RTCtime;

//wait for the second to roll over:
while((millis() - localTime.millistimestamp + localTime.milliseconds) % 1000 < 999)
{
  yield();
}
  
  uint32_t epoch = (localTime.NTPtime - 2208988800UL) + (millis() - localTime.millistimestamp + localTime.milliseconds) / 1000;
  if (((millis() - localTime.millistimestamp + localTime.milliseconds) % 1000) >= 500) epoch++; //round up if needed
  RTCtime.InitWithEpoch32Time(epoch);
  RTC.SetDateTime(RTCtime);

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
  


}


void RTCinit(void)
{
  Wire.begin(); //default pins: 4 & 5
  RTC.Begin();
  RTC.Enable32kHzPin(false);
  RTC.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

/*
  if (updatelocaltimefromRTC() > 0)
  {
    Serial.println(F("RTC time not valid"));
    RTCTimeValid = false;
  }
  else
  {
    RTCTimeValid = true;
  }*/
}


