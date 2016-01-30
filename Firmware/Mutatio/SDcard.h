#define SD_CSN_PIN 16
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )  //from time library

void SDwriteLogfile(String entry)
{
  if (config.useSDcard)
  {
    String sfilename = "logfile.txt";
    RtcDateTime RTCtime  = RTC.GetDateTime(); //get current time from RTC

    char filename[sizeof(sfilename)];
    sfilename.toCharArray(filename, sizeof(filename));

    //todo: check if the file exists, write a header if it does not
    yield();
    // open the file
    File dataFile = SD.open(filename, FILE_WRITE);
    if (dataFile) {
      sdWatchdog = 0;
      String datatoWrite = String(RTCtime.Day()) + ". " + String(RTCtime.Month()) + ". " + String(RTCtime.Year()) + " " + String(RTCtime.Hour()) + ":" + String(RTCtime.Minute()) + ":" + String(RTCtime.Second()); //the timestamp in seconds
      datatoWrite += "\t";
      datatoWrite += entry; //how many good values went into this measurment
      dataFile.println(datatoWrite);
      dataFile.close();
      Serial.println("Logfile Written");
    }
  }
}

uint8_t SDwriteMeasurements(uint8_t count) //writes unwritten measurements to SD card, one file per day (filename is the date)
{
  //the time is logged in UNIX time, seconds since Jan 1. 1970
  //get the first timestamp and open or create the file with the name year-day

  /*
    //problem with the following code: generates filename longer than 12 characters, which will fail to open
      uint32_t epoch  =  measurementdata[datareadindex].Timestamp - 2208988800UL;
    RtcDateTime timeelements;
    timeelements.InitWithEpoch32Time(epoch); //converte epoch to date, month etc.

    String sfilename = String(timeelements.Year()) +"-" +  String(timeelements.Month()) + "-" +  String(timeelements.Day())+ ".txt";
    Serial.print("Filename: ");
    Serial.println(sfilename);


  */

  if (config.useSDcard)
  {
    uint32_t UNIXtime = measurementdata[datareadindex].Timestamp - 2208988800UL;

    //caclulate the year (calculation from time library)
    uint16_t year = 0;
    uint32_t day =  UNIXtime / 86400UL; //full days since Jan 1. 1970
    uint32_t tempdays = 0; //count the days in full years
    while ((unsigned)(tempdays += (LEAP_YEAR(year) ? 366 : 365)) <= day) {
      year++;
    }
    //now we have the full years since 1970 and the number of days in those ears stored in tempday.
    //deduct the days added in the last loop from tempday and remove the remaining days from the days since 1970
    //and we get the days in this year starting from 0, so add 1 to start at day 1

    tempdays -= LEAP_YEAR(year) ? 366 : 365;
    year += 1970;
    day  -= tempdays;
    day++; //add one day to start at day 1 in a year

    String sfilename = String(year) + "--" + String(day) + ".txt";
    Serial.print("Filename: ");
    Serial.println(sfilename);


    char filename[sizeof(sfilename)];
    sfilename.toCharArray(filename, sizeof(filename));

    //todo: check if the file exists, write a header if it does not
    yield();
    // open the file
    File dataFile = SD.open(filename, FILE_WRITE);
    File unsentDataFile;


    // if the file is available, write to it:
    if (dataFile) {
      uint16_t j;
      uint16_t i = datareadindex;
      sdWatchdog = 0;
      for (j = datareadindex; j < (uint16_t)count + datareadindex; j++) //check first if any data is pending to be sent to server
      {
        if (measurementdata[i].flag > 0 && ((measurementdata[i].flag & 0x08) == 0)) //data not yet sent to server
        {
          unsentDataFile = SD.open(unsentFilename, FILE_WRITE);
          break;
        }
        i++;
        if (i >= (MEASUREMENTVALUES)) i = 0; //continue at beginning if end of buffer reached
      }


      for (j = datareadindex; j < (uint16_t)count + datareadindex; j++) //go through all available measurements and write them to SD
      {
        yield();
        if (measurementdata[i].flag > 0 && ((measurementdata[i].flag & 0x04) == 0)) //data not yet written to SD
        {

          /*
                  //save unsent data in special file
                  if ((measurementdata[i].flag & 0x08) == 0) //not yet sent to server, save a backup
                  {
                    String unsentdatastring = "1 " + getJsonFromMeasurement(measurementdata[i]) + "\r\n";
                    unsentDataFile.print(unsentdatastring);
                    unsentSDData = 1; //unsent data is now pending
                    measurementdata[i].flag |= 0x08; //set sent out flag, this data is backed up now, will be sent when backup data is sent out
                    // Serial.println(F("Unsent data written to SD"));
                  }
          */

          char temparr[12] = {0};
          String datapoint;
          datapoint = dtostrf(((float)measurementdata[i].data / 100000) + 50, 10, 5, temparr);
          // datapoint.trim(); //trim whitespaces


          String datatoWrite = String(measurementdata[i].Timestamp - 2208988800UL); //the UTC timestamp in seconds (Epoch Unix time format)
          datatoWrite += ".";
          datatoWrite += String(measurementdata[i].milliseconds); //the timestamps milliseconds
          datatoWrite += "\t";
          datatoWrite += datapoint;
          datatoWrite += "\t";
          datatoWrite += String(measurementdata[i].quality); //how many good values went into this measurment
          datatoWrite += "\t";
          datatoWrite += String(measurementdata[i].flag); //the info flag
          datatoWrite += "\r\n";
          uint16_t byteswritten = dataFile.print(datatoWrite);

          if (byteswritten >= datatoWrite.length())
          {

            //ok, clear the flag
            if (measurementdata[i].flag & 0x08) measurementdata[i].flag = 0; //already sent out? reset the flag to zero
            else measurementdata[i].flag |= 0x04; //not sent out, mark this.
            datareadindex = i;
          }
          else
          {
            Serial.println(F("error writing to file"));
            yield();
            dataFile.close();
            return -1;
          }
        }
        i++;
        if (i >= (MEASUREMENTVALUES)) i = 0; //continue at beginning if end of buffer reached
      }
      unsentDataFile.close();
      dataFile.close();
      Serial.println(F("data written to SD"));
    }
    // if the file cannot be opened:
    else {
      Serial.println(F("error opening file"));
      yield();
      dataFile.close();
      return -1;
    }
  }

  return 0;
}



void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

uint8_t SDinit(uint8_t pin)
{

  yield();
  unsentSDData = 0;
  SD_initialized = 0;
  sdWatchdog = 0;
  if (config.useSDcard)
  {
    //check if an unsent data file exists on the SD card
    if (SD.exists((char *)unsentFilename.c_str()))
    {
      unsentSDData = 1;
    }

    pinMode(SD_CSN_PIN, OUTPUT);
    digitalWrite(SD_CSN_PIN, HIGH);
    Serial.print(F("SD init... "));
    //check SD detect pin, sd is present if it is low
    if (analogRead(A0) < 50)
    {
      sdWatchdog = 0;
      if (!SD.begin(pin)) {
        yield();
        Serial.println(F("Card failed, rebooting..."));
        delay(100);
        ESP.restart(); //if the card init failes, only a reboot helps. todo: make this work without a reboot(bug in the library?)
        return -1;
      }
      else
      {
        SD_initialized = 1;
        Serial.println(F("SD card initialized."));
        File dir = SD.open("/");
        printDirectory(dir, 0);
        return 0;
      }
    }
    else
    {
      Serial.println(F("Card not present"));
      SD_initialized = 0;
      sdWatchdog = 1;
    }
  }
}

