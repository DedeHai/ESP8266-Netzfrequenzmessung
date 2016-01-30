void initOTAupdate(void)
{
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("Mutatio");
  // No authentication by default
  //ArduinoOTA.setPassword("abc");
  ArduinoOTA.onStart([]() {
    detachInterrupt(MEASUREMENTPIN);
    LEDcolor.r = 100;
    LEDcolor.g = 0;
    LEDcolor.b = 100;
    LED.setPixelColor(0, LED.Color(LEDcolor.r, LEDcolor.g, LEDcolor.b));
    LED.show(); // show LED color
    Serial.println(F("OTA update..."));
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA update finished");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

}

