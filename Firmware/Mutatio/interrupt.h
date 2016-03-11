#define ALLOWEDNOISE 700 //accepted noise in ticks 
/*
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

*/

void ICACHE_RAM_ATTR pininterrupt() {

  timekeeper = ESP.getCycleCount();


  float frequencyoffset;
  int32_t noise;
  static float lowpassfiltered = 1600000;
  static uint32_t capturetimeaverage = 1600000;
  static uint32_t lastcapture_rising;
  static uint32_t lastcapture_falling;
  static uint8_t datawriteindex = 0;

  if (digitalRead(MEASUREMENTPIN) == HIGH) //digitalRead could be made faster using GPIO_REG_READ, but digitalRead only takes 3µs anyways, which is fast enough for this slow interrupt
  {
    //rising edge
    capturetime[datawriteindex] = timekeeper - lastcapture_rising; //calculate the ticks since last interrupt (unsigned integers, correct even at overflows)
    lastcapture_rising = timekeeper;
  }
  else
  {
    //falling edge
    capturetime[datawriteindex] = timekeeper - lastcapture_falling; //calculate the ticks since last interrupt (unsigned integers, correct even at overflows)
    lastcapture_falling = timekeeper;
  }


  //check for valid time: must be 1600000±10% (~20ms)
  noise = ((int32_t)capturetime[datawriteindex] - (int32_t)1600000);
  if ((noise < 160000) && (noise > -160000))
  {
    lowpassfiltered = ((float)capturetime[datawriteindex] *  0.07) + (lowpassfiltered  * 0.93);
    datawriteindex++; //noise seems ok, keep this value
    signalWatchdog = 0;
  }

  if (datawriteindex >= NUMBEROFCAPTURES)
  {
    uint16_t i;
    uint8_t baddataA = 0;
    uint8_t baddataB = 0;
    int32_t noiseA, noiseB;
    uint32_t ticksA = 0;
    uint32_t ticksB = 0;


    //double check for noise, keep the tolerance tight, compare to lowpass filtered value and to last value, pick whichever comparison shows less noise
    for (i = 0; i < NUMBEROFCAPTURES; i++)
    {
      noiseA = (capturetime[i] - (int32_t)lowpassfiltered);
      noiseB = (capturetime[i] - capturetimeaverage);
      if (noiseA < ALLOWEDNOISE && noiseA > -ALLOWEDNOISE)
      {
        ticksA += capturetime[i];
      }
      else
      {
        baddataA++; //too much noise in this datapoint
      }
      if (noiseB < (ALLOWEDNOISE + 50) && noiseB > -(ALLOWEDNOISE + 50))
      {
        ticksB += capturetime[i];
      }
      else
      {
        baddataB++; //too much noise in this datapoint
      }
    }

    //check which one is better, swap values if needed
    if (baddataA > baddataB)
    {
      baddataA = baddataB;
      ticksA = ticksB;
    }


    capturetimeaverage = ((float)ticksA / (NUMBEROFCAPTURES - baddataA)); //save the value for next time

    frequencyoffset = ((((float)FCPU + config.FCPUerror) / capturetimeaverage) - 50.0) * 100000; //frequency offset to 50.000Hz in [mHz]*100
    float lowpassmeasurement = ((((float)FCPU +  config.FCPUerror) / (lowpassfiltered)) - 50.0) * 100000;
    writeMeasurement((int32_t)frequencyoffset, (uint8_t) (NUMBEROFCAPTURES - baddataA) , (int32_t)lowpassmeasurement);


    datawriteindex = 0;
    issampling = false;
    //Serial.print("K");

  }

  lastcapture = timekeeper; //save the captured time for other functions (i.e. LED)
}


