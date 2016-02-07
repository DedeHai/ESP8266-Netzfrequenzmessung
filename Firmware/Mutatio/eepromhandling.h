
void WriteStringToEEPROM(int beginaddress, String string)
{
  if (string.length() > 31) //allocated space in eeprom for each string is 32bytes: 31bytes for string, 1byte for terminating zero
  {
    return; //quit the function, the string cannot be longer than 31 chars (bad input)
  }

  char  charBuf[string.length() + 1];
  string.toCharArray(charBuf, string.length() + 1);
  for (int t =  0; t < sizeof(charBuf); t++)
  {
    EEPROM.write(beginaddress + t, charBuf[t]);
  }
}

String  ReadStringFromEEPROM(int beginaddress)
{
  byte counter = 0;
  char rChar;
  String retString = "";
  while (1)
  {
    rChar = EEPROM.read(beginaddress + counter);
    if (rChar == 0) break;
    if (counter > 31) break;
    counter++;
    retString.concat(rChar);

  }
  return retString;
}


void WriteConfig()
{

  Serial.println(F("Writing Config"));
  EEPROM.write(0, 'C');
  EEPROM.write(1, 'F');
  EEPROM.write(2, 'G');

  EEPROM.write(3, config.useDHCP);
  EEPROM.write(4, config.useSDcard);
  EEPROM.write(5, config.useRTC);
  EEPROM.write(6, config.sendAllData);
  EEPROM.write(7, (uint8_t)(config.FCPUerror & 0xFF)); //low byte
  EEPROM.write(8, (uint8_t)((config.FCPUerror >> 8) & 0xFF)); //high byte
  EEPROM.write(9, (uint8_t)(config.serverport & 0xFF)); //low byte
  EEPROM.write(10, (uint8_t)((config.serverport >> 8) & 0xFF)); //high byte

  EEPROM.write(16, config.IP[0]);
  EEPROM.write(17, config.IP[1]);
  EEPROM.write(18, config.IP[2]);
  EEPROM.write(19, config.IP[3]);

  EEPROM.write(20, config.Netmask[0]);
  EEPROM.write(21, config.Netmask[1]);
  EEPROM.write(22, config.Netmask[2]);
  EEPROM.write(23, config.Netmask[3]);

  EEPROM.write(24, config.Gateway[0]);
  EEPROM.write(25, config.Gateway[1]);
  EEPROM.write(26, config.Gateway[2]);
  EEPROM.write(27, config.Gateway[3]);

  WriteStringToEEPROM(32, config.APIkey);
  WriteStringToEEPROM(64, config.ssid);
  WriteStringToEEPROM(96, config.password);
  WriteStringToEEPROM(128, config.DeviceName);
  WriteStringToEEPROM(160, config.DevicePW);
  WriteStringToEEPROM(192, config.serveraddress);
  WriteStringToEEPROM(224, config.serverURI);


  EEPROM.commit();
}

boolean ReadConfig()
{

  Serial.println(F("Reading Configuration"));
  if (EEPROM.read(0) == 'C' && EEPROM.read(1) == 'F'  && EEPROM.read(2) == 'G' )
  {
    Serial.println("Configurarion Found!");
    config.useDHCP =   EEPROM.read(3);
    config.useSDcard =   EEPROM.read(4);
    config.useRTC =   EEPROM.read(5);
    config.sendAllData =   EEPROM.read(6);
    int16_t lowbyte =   EEPROM.read(7);
    int16_t highbyte =   EEPROM.read(8);
    config.FCPUerror = (lowbyte & 0xFF) + ((highbyte << 8) & 0xFFFF);
    lowbyte =   EEPROM.read(9);
    highbyte =   EEPROM.read(10);
    config.serverport = (lowbyte & 0xFF) + ((highbyte << 8) & 0xFFFF);

    config.IP[0] = EEPROM.read(16);
    config.IP[1] = EEPROM.read(17);
    config.IP[2] = EEPROM.read(18);
    config.IP[3] = EEPROM.read(19);
    config.Netmask[0] = EEPROM.read(20);
    config.Netmask[1] = EEPROM.read(21);
    config.Netmask[2] = EEPROM.read(22);
    config.Netmask[3] = EEPROM.read(23);
    config.Gateway[0] = EEPROM.read(24);
    config.Gateway[1] = EEPROM.read(25);
    config.Gateway[2] = EEPROM.read(26);
    config.Gateway[3] = EEPROM.read(27);

    config.APIkey = ReadStringFromEEPROM(32);
    config.ssid = ReadStringFromEEPROM(64);
    config.password = ReadStringFromEEPROM(96);
    config.DeviceName = ReadStringFromEEPROM(128);
    config.DevicePW = ReadStringFromEEPROM(160);
    config.serveraddress =  ReadStringFromEEPROM(192);
    config.serverURI =  ReadStringFromEEPROM(224);


    return true;

  }
  else
  {
    Serial.println(F("Configurarion NOT FOUND!!!!"));
    return false;
  }
}


void writeDefaultConfig(void)
{
  // DEFAULT CONFIG
  config.ssid = "MYSSID";
  config.password = "MYPASSWORD";
  config.IP[0] = 192; config.IP[1] = 168; config.IP[2] = 1; config.IP[3] = 44;
  config.Netmask[0] = 255; config.Netmask[1] = 255; config.Netmask[2] = 255; config.Netmask[3] = 0;
  config.Gateway[0] = 192; config.Gateway[1] = 168; config.Gateway[2] = 1; config.Gateway[3] = 1;

  //Use MAC address to generate a uique ID like "Mutatio-7F89"
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  config.DeviceName = "Mutatio-" + macID;
  config.DevicePW = "11111111";
  config.APIkey = "";
  config.useDHCP = true;
  config.useSDcard = false;
  config.useRTC = false;
  config.sendAllData = false;
  config.FCPUerror = 0;

  config.serveraddress =  "api.netzsin.us";
  config.serverport = 8080;
  config.serverURI = "/api/submit/meter1";
  

  
  WriteConfig();
  Serial.println(F("Standard config applied"));
}
