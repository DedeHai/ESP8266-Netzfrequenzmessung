
/*
//synchronize the local clock with the NTP server's
void NTP_gettime(localTime t)
{
  
  Serial.print(F("NTP Sync"));
  uint32_t NTP_start_time;
  uint32_t timeout = millis();
  uint32_t millistimestamp;
  uint8_t i = 0;
  uint8_t errorcounter = 0;
  uint8_t responsecounter = 0; //successfully received responses
  int gotresponse = 0;
  int timedifference = 0;
  unsigned long secsSince1900; //NTP time
  int millisecondtime; //NTP second fractions in milliseconds
  while (i < REQUESTSTOAVERAGE && errorcounter < 5)
  {
    WiFi.hostByName(ntpServerName, timeServerIP);
    //empty the cache:
    while (udp.parsePacket())
    {
      udp.flush();
    }
    timeout = millis();
    sendNTPpacket(timeServerIP, &NTP_start_time); // send an NTP packet to a time server
    // wait to see if a reply is available
    do {
      yield();
      gotresponse = udp.parsePacket();
    } while (!gotresponse && (millis() - timeout < 300)); //do not set timeout bigger than 999ms (overflow of milliseconds in below calculation)

    if (gotresponse == NTP_PACKET_SIZE) {

      millistimestamp = millis(); //save the current millis() time at reception of packet
      responsecounter++;
      // Serial.println("NTP response time = " + String(millistimestamp - NTP_start_time) + "[ms]");
      // We've received a packet, read the data from it
      udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer


      //the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      secsSince1900 = highWord << 16 | lowWord;

      //extract the fraction of the second:
      millisecondtime =  (int)((((long)packetBuffer[44] * 256 + packetBuffer[45]) / 65536.0 + 0.0005) * 1000); //fraction in milliseconds
      //add half of the roundtrip delay time:
      millisecondtime = millisecondtime + (millistimestamp - NTP_start_time) / 2;

      //check if this got us to the next full second:
      if (millisecondtime >= 1000)
      {
        secsSince1900++;
        millisecondtime -= 1000;
      }

      if (current_time == 0) //first time, save to local clock
      {
        current_time_ms = millisecondtime;
        current_time = secsSince1900;
        current_time_millistimestamp = millistimestamp;
      }
      else //show the offset of local clock to NTP time:
      {
        //only use good packets i.e. if the response was faster than 30ms or else the time uncertainty is too high
        if (millistimestamp - NTP_start_time < 30)
        {
          uint64_t nowmilliseconds = (uint64_t)current_time * 1000 + current_time_ms + (millis() - current_time_millistimestamp);
          uint64_t NTPmilliseconds = (uint64_t)secsSince1900 * 1000 + millisecondtime;
          int32_t millisdifference = int32_t(nowmilliseconds - NTPmilliseconds);
          // Serial.println("Timedifference = " + String(millisdifference));
          Serial.print(".");
          if (millisdifference < 500 && millisdifference > -500)
          {
            timedifference += millisdifference;
            i++;
          }

        }
      }


    }
    else {
      errorcounter++;
      // Serial.println("no packet received");
    }

    delay(50); //do not request too fast or server will not respond to request


  }
  
  printUTCtime(secsSince1900);
  Serial.print("Average Timedifference = ");
  Serial.println((float)timedifference / responsecounter);

  if(timedifference>=0)
  {
  timedifference = (int)((float)timedifference / responsecounter + 0.5);
  }
  else
  {
     timedifference = (int)((float)timedifference / responsecounter - 0.5);
  }

  
  if (updatelocaltime)
  {

    millisecondtime += timedifference; //both are signed ints, so no problem with overflow
    if (millisecondtime >= 1000)
    {
      secsSince1900++;
      millisecondtime -= 1000;
    }
    else if (millisecondtime < 0)
    {
      secsSince1900--;
      millisecondtime += 1000;
    }
    current_time_ms = millisecondtime;
    current_time = secsSince1900;
    current_time_millistimestamp = millistimestamp;
    Serial.println("Local Time Updated");
    updatelocaltime = false;

  }


}*/
