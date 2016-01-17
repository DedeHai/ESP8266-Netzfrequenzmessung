#define SD_CSN_PIN 16
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )  //from time library



uint8_t SDwriteMeasurements(uint8_t count) //writes unwritten measurements to SD card, one file per day (filename is the date)
{
  //the time is logged in UNIX time, seconds since Jan 1. 1970
  //get the first timestamp and open or create the file with the name year-day

  unsigned long UNIXtime = measurementdata[datareadindex].Timestamp - 2208988800UL;

  //caclulate the year (calculation from time library)
  uint16_t year = 0;
  uint32_t day =  UNIXtime / 86400; //full days since Jan 1. 1970
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

  String sfilename = String(year) + "-" + String(day) + ".CSV";
  Serial.print("Filename: ");
  Serial.println(sfilename);


  char filename[sizeof(sfilename)];
  sfilename.toCharArray(filename, sizeof(filename));

  //todo: check if the file exists, write a header if it does not
  yield();
  // open the file
  File dataFile = SD.open(filename, FILE_WRITE);



  // if the file is available, write to it:
  if (dataFile) {
    uint16_t j;
    uint8_t i = datareadindex;

    for (j = datareadindex; j < (uint16_t)count + datareadindex; j++) //go through all available measurements and write them to SD
    {
      yield();
      if (measurementdata[i].flag > 0 && ((measurementdata[i].flag & 0x04) == 0)) //data not yet written to SD
      {
        String datatoWrite = String(measurementdata[i].data / 10); //the measurement data
        datatoWrite += ",";
        datatoWrite += String(measurementdata[i].Timestamp); //the timestamp in seconds
        datatoWrite += ",";
        datatoWrite += String(measurementdata[i].milliseconds); //the timestamps milliseconds
        datatoWrite += ",";
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
  pinMode(SD_CSN_PIN, OUTPUT);
  digitalWrite(SD_CSN_PIN, HIGH);
  
  //check SD detect pin, sd is present if it is low
  if (analogRead(A0) < 50)
  {
    if (!SD.begin(pin)) {
      yield();
      Serial.println(F("Card failed"));
      SD_initialized = 0;
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
  }
}

