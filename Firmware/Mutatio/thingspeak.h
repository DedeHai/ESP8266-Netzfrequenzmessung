//Thingspeak sendout, do not send more often than every 15 seconds
//used for debugging purposes

uint8_t updateThingspeak(int data)
{
 
  bool thingspeakOK = false;
  //checks if we can already send more data. if we can, check if data needs to be sent.
  //then format the data and send it out through the ether

  // Use WiFiClient class to create TCP connections
  char temparr[12];
  String datapoint;
  datapoint = dtostrf((float)data / 100, 10, 2, temparr);
  datapoint.trim(); //trim whitespaces
  //Serial.println(datapoint);
  String datastr = "";
  String header = "";
  datastr += "field";
  datastr += String(1);
  datastr += "=";
  datastr += datapoint;//String(data);

  Serial.print(F("Thingspeak..."));
  if (!client.connect("api.thingspeak.com", 80)) {
    Serial.println(F("Thingspeak connect failed"));
    client.stop();
    return -1;
  }
  header += "POST /update HTTP/1.1\r\n";
  header += "Host: api.thingspeak.com\r\n";
  header += "Connection: close\r\n";
  header += "X-THINGSPEAKAPIKEY: ";
  header += config.APIkey;
  header += "\r\n";
  header += "Content-Type: application/x-www-form-urlencoded\r\n";
  header += "Content-Length: ";
  header += String(datastr.length());
  header += "\r\n\r\n";
  client.print(header);
  client.print(datastr);
  int timeout = 0;
  while (client.available() == 0)
  {
    delay(10);
    if (timeout++ > 200) break; //wait 2s max
  }
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (line.indexOf("Status: 200 OK") != -1)
    {
      //thingspeakflags = 0; //reset flags after sending
      Serial.println("OK");
      thingspeakOK = true;
      thingspeakTime = millis();
    }

    //  Serial.print(line); //print out what we received
  }

  client.flush();
  client.stop();
  if ( thingspeakOK == false)
  {
    Serial.println("FAIL");
    return -1;
  }
  else return 0;

}

