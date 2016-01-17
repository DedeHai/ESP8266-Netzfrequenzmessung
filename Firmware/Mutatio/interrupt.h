
void ICACHE_RAM_ATTR pininterrupt() {

  float frequencyoffset;
  int32_t noise;
  static uint8_t init = 10;
  static uint32_t lastdifference = 0;
  timekeeper = ESP.getCycleCount();
  
  if(init)
  {
    init--;  
  }
  if (timekeeper > lastcapture) //check 32bit integer overflow integer overflow
  {
    Serial.println( ESP.getCycleCount() - lastcapture); //calculate the ticks since last interrupt
  }
  else
  {
    Serial.println(lastcapture - ESP.getCycleCount()); //calculate the ticks since last interrupt
  }

  lastcapture = timekeeper; //save the captured time for next time
}


/*
void ICACHE_RAM_ATTR pininterrupt() {

  float frequencyoffset;
  int32_t noise;
  timekeeper = ESP.getCycleCount();

  if (timekeeper > lastcapture) //check 32bit integer overflow integer overflow
  {
    capturetime[datawriteindex] = ESP.getCycleCount() - lastcapture; //calculate the ticks since last interrupt
  }
  else
  {
    capturetime[datawriteindex] = lastcapture - ESP.getCycleCount(); //calculate the ticks since last interrupt
  }

  //check for valid time: must be 1600000±10%
  noise = ((int32_t)capturetime[datawriteindex] - (int32_t)1600000);
  if (noise < 160000 && noise > -160000)
  {
    datawriteindex++; //noise seems ok, keep this value
    //Serial.print("o");
  }
  else
  {
    // Serial.print("n");
  }

  if (datawriteindex >= NUMBEROFCAPTURES)
  {
    uint16_t i;
    uint8_t baddata = 0;
    int32_t noise;
    uint32_t ticks = 0;
    uint16_t referenceticks = (capturetime[NUMBEROFCAPTURES - 5] + capturetime[NUMBEROFCAPTURES / 2] + capturetime[NUMBEROFCAPTURES / 30]) / 3;

    //double check for noise, keep the tolerance tight (2us), compare to average of some samples
    for (i = 0; i < NUMBEROFCAPTURES; i++)
    {
      noise = ((int32_t)capturetime[i] - (int32_t)referenceticks);
      if (noise < 160 && noise > -160)
      {
        baddata++; //too much noise in this datapoint
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

    datawriteindex = 0;
    issampling = false;
    //Serial.print("K");
  }

  lastcapture = timekeeper; //save the captured time for next time
}*/

