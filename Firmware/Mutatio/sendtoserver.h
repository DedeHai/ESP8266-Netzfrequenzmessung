
uint8_t sendStringToServer(String jsonstring)
{
  
  bool ServerOK = false;
  //checks if we can already send more data. if we can, check if data needs to be sent.
  //then format the data and send it out through the ether
  
  if (!client.connect(config.serveraddress.c_str(), config.serverport)) {
    Serial.println(F("Server connect failed"));
    client.flush();
    client.stop();
    return -1;
  }

 // Serial.print("Sendoutdata: ");
 // Serial.println(jsonstring);

  String msglength = String(jsonstring.length());
 
  String header = "POST "+  config.serverURI + " HTTP/1.1\r\n";
  header += "Host: " + config.serveraddress + ":" + String(config.serverport) + "\r\n";
  header += "User-Agent: Mutatio/1.0\r\n";
  header += "X-API-KEY: ";
  header += config.APIkey; // "secretkey1";
  header += "\r\nContent-Type: application/json\r\n";
  header += "Connection: close\r\n";
  header += "Content-Length: ";
  header += msglength;
  header += "\r\n\r\n";

  //Serial.print(header);
  //Serial.print(jsonstring);
  client.print(header);
  client.print(jsonstring);

 
  int timeout = 0;
  while (client.available() == 0)
  {
    delay(10);
    if (timeout++ > 50) break; //wait .5s max
  }
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
     //Serial.println(line); //uncomment this line to see servers response on serial
    if (line.indexOf("200 OK") != -1)
    {
      // Serial.println(F("Server OK"));
      ServerOK = true;
      break;
    }    
  }
  client.flush();
  client.stop();

  if (ServerOK == false)
  {
    Serial.println(F("Server FAILED"));
    return -1;
  }
  else return 0;
}


uint8_t sendMeasurementToServer(Measurement datastruct)
{
  String jsondata = getJsonFromMeasurement(datastruct);
  return sendStringToServer(jsondata);
}

void sendSDdataToServer(void)
{
  uint16_t linecounter = 0; //number of lines read from file
  uint16_t successcounter = 0; //number of successfully sent lines
  uint32_t takeabreak = millis();
  static uint32_t lastreadposition = 0;

  if (SD.exists((char *)unsentFilename.c_str()))
  {
    //open the file for read&write, start reading at the beginning
    //first char in a line is the 'pending' flag, it is '1' when a line still needs to be sent, '0' if not
    //if all data was sent out, delete the file
    File dataFile = SD.open(unsentFilename, FILE_WRITE);
    if (lastreadposition > dataFile.size()) //if true, a new file was written since last call
    {
      lastreadposition = 0;
    }
    dataFile.seek(lastreadposition); //go to reading position or
    dataFile.setTimeout(50); //timeout for streams, file reading should be fast, set timeout low to not cause WDT resets
    uint32_t writeposition = 0;
    while (dataFile.available() > 10)
    {
      yield();
      writeposition = dataFile.position(); //first byte in this line (flag), we will overwrite it with '0' once data is sent out
      if (dataFile.read() == '1') //pending flag is set
      {
        linecounter++;
        dataFile.read(); //discard the space char
        String line = dataFile.readStringUntil('}') + "}"; //add the '}' at the end, it is not included int the readStringUntil

        uint32_t readposition = dataFile.position(); //save it to jump back after writing
        //send the string to the server
        uint8_t failed = sendStringToServer(line);

        //if sending is successful, write the first byte in the line to '0'
        if (failed == 0)
        {
          // Serial.println(line);
          successcounter++;
          dataFile.seek(writeposition);
          dataFile.write('0');
          dataFile.seek(readposition);
        }
        else
        {
          break; //server connection failed, try again later
        }
      }
      else //line has already been sent out, seek forward to beginning of next line (i.e. find '\n')
      {
        while (dataFile.available())
        {
          if (dataFile.read() == '\n')
          {
            lastreadposition = dataFile.position();
            break;
          }
        }
      }

      if (millis() - takeabreak > 40000)
      {
        dataFile.close();
        return; //take a break after 20 seconds to make sure current data gets saved to SD if this operation takes a long time
      }
    }

    dataFile.close();
    // delay(10); //wait for the file to be closed
    //if all data sent, delete the file:
    if (linecounter == successcounter)
    {
      SD.remove((char *)unsentFilename.c_str());
      unsentSDData = 0;
      Serial.println(F("all data sent, file deleted"));
    }
  }
}

