
// convert a single hex digit character to its integer value (from https://code.google.com/p/avr-netino/)
unsigned char h2int(char c)
{
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

//decode string from url-arguments
String urldecode(String input) // (based on https://code.google.com/p/avr-netino/)
{
  char c;
  String ret = "";

  for (byte t = 0; t < input.length(); t++)
  {
    c = input[t];
    if (c == '+') c = ' ';
    if (c == '%') {


      t++;
      c = input[t];
      t++;
      c = (h2int(c) << 4) | h2int(input[t]);
    }

    ret.concat(c);
  }
  return ret;

}


void sendPage(void)
{

  Serial.println("HTML Page");

  if (server.args() > 0 )  // Save Settings
  {
    uint8_t i;
    //reset all boolean data (is set true if checked value is received)
    config.useDHCP = false;
    config.useSDcard = false;
    config.useRTC = false;
    config.sendAllData = false;

    for (i = 0; i < server.args(); i++ ) {

      if (server.argName(i) == "SSID") config.ssid = urldecode(server.arg(i));
      if (server.argName(i) == "PASS") config.password = urldecode(server.arg(i));
      if (server.argName(i) == "API_KEY")  config.APIkey = server.arg(i);
      if (server.argName(i) == "AP_NAME") config.DeviceName = urldecode(server.arg(i));
      if (server.argName(i) == "AP_PASS") config.DevicePW = urldecode(server.arg(i));
      if (server.argName(i) == "DHCP") config.useDHCP = true;
      if (server.argName(i) == "USE_SD") config.useSDcard = true;
      if (server.argName(i) == "USE_RTC") config.useRTC = true;
      if (server.argName(i) == "S_ALL") config.sendAllData = true;
    }
    WriteConfig();
  }

  String pageContent = "<HTML><HEAD> <title>Mutatio Configuration</title></HEAD>\n";
  pageContent += "<BODY bgcolor=\"#ffcc00\" text=\"#000000\">";
  pageContent += "<FONT size=\"6\" FACE=\"Verdana\">\n<BR><b>Mutatio Configuration</b><BR><BR></font>";
  pageContent += "<form action=\"\" method=\"GET\">\n";
  pageContent += "<b>WiFi Configuration</b><BR><BR>\n";
  pageContent += "<input type=\"text\"  size=\"24\" maxlength=\"31\" name=\"SSID\" value=\"";
  pageContent += config.ssid;
  pageContent += "\"> SSID&emsp;&emsp;&emsp;&emsp;WiFi Status: ";

  if (WiFi.status() == WL_CONNECTED)
  {
    pageContent += "CONNECTED. IP= " +  WiFi.localIP().toString() + "<BR>\n";
  }
  else
  {
    pageContent += "NOT CONNECTED<BR>\n";
  }

  pageContent += "<input type=\"password\"  size=\"24\" maxlength=\"31\" name=\"PASS\" value=\"";
  pageContent += config.password;
  pageContent += "\"> Password<BR>\n";
  //pageContent += "<input type=\"submit\" value=\"Send\"><BR></form>";

  //todo: add DHCP configuration possibility here

  pageContent += "<INPUT TYPE=\"CHECKBOX\" NAME=\"DHCP\" VALUE=\"1\" ";
  if (config.useDHCP) pageContent += " CHECKED>";
  else pageContent += ">";
  pageContent += " Use DHCP<BR>\n";

  //todo: add fields for IP and stuff if DHCP is not used
  /*
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
  */

  pageContent += "<HR>"; //horizontal line-----------------------------------------------------------------------

  // pageContent += "<form action=\"\" method=\"GET\">\n";
  pageContent += "<b>Mutatio Configuration</b><BR><BR>\n";

  pageContent += "<input type=\"text\" size=\"28\" maxlength=\"16\" name=\"API_KEY\" value=\"";
  pageContent += config.APIkey;
  pageContent += "\"> Server API Key <BR>\n";
  // pageContent += "<input type=\"submit\" value=\"Send\"><BR></form>";

  pageContent += "<input type=\"text\" size=\"28\" maxlength=\"32\" name=\"AP_NAME\" value=\"";
  pageContent += config.DeviceName;
  pageContent += "\"> AccesPoint device name <BR>\n";

  pageContent += "<input type=\"text\" size=\"28\" maxlength=\"32\" name=\"AP_PASS\" value=\"";
  pageContent += config.DevicePW;
  pageContent += "\"> AccesPoint password <BR>\n";

  pageContent += "<INPUT TYPE=\"CHECKBOX\" NAME=\"USE_SD\" VALUE=\"1\" ";
  if (config.useSDcard) pageContent += " CHECKED>";
  else pageContent += ">";
  pageContent += " Use SD-card<BR>\n";

  pageContent += "<INPUT TYPE=\"CHECKBOX\" NAME=\"USE_RTC\" VALUE=\"1\" ";
  if (config.useRTC) pageContent += " CHECKED>";
  else pageContent += ">";
  pageContent += " Use real time clock (RTC)<BR>\n";

  pageContent += "<INPUT TYPE=\"CHECKBOX\" NAME=\"S_ALL\" VALUE=\"1\" ";
  if (config.sendAllData) pageContent += " CHECKED>";
  else pageContent += ">";
  pageContent += " Send all data to Server (not only most recent value)<BR>\n";


  pageContent += "<BR>\n";

  pageContent += "<input type=\"submit\" value=\"Send\"><BR></form>";
  pageContent += "<HR>"; //horizontal line-----------------------------------------------------------------------


  //todo: add status of node here (SDcard, RTC, Signal)

  server.send ( 200, "text/html", pageContent );

}

void handleNotFound() {
  String message = "404 Not Found:";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println(message);
}


