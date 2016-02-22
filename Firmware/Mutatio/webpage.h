
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
    uint8_t i, j;
    //reset all boolean data (is set true if checked value is received)
    config.useDHCP = false;
    config.useSDcard = false;
    config.useRTC = false;
    config.sendAllData = false;

    for (i = 0; i < server.args(); i++ ) {


      if (server.argName(i) == "SSID") config.ssid = urldecode(server.arg(i));
      if (server.argName(i) == "PASS") config.password = urldecode(server.arg(i));

      //ip, gateway and subnet extraction from input strings:
      int8_t pos = 0;
      if (server.argName(i) == "IP")
      {
        for (j = 0; j < 4; j++ ) //a valid ip is in the form "192.168.1.2", look for this pattern
        {
          String number = server.arg(i).substring(pos);    // get  characters (toInt ignores non integer values)
          config.IP[j] = (uint8_t)(number.toInt() & 0xFF);
          pos = server.arg(i).indexOf(".", pos);
          if (pos < 0) break; //no dot found, this is an invalid address or we are done here
          pos++; //start reading after the "."
        }
      }
      pos = 0;
      if (server.argName(i) == "NM")
      {
        for (j = 0; j < 4; j++ ) //a valid ip is in the form "192.168.1.2", look for this pattern
        {
          String number = server.arg(i).substring(pos);    // get  characters (toInt ignores non integer values)
          config.Netmask[j] = (uint8_t)(number.toInt() & 0xFF);
          pos = server.arg(i).indexOf(".", pos);
          if (pos < 0) break; //no dot found, this is an invalid address or we are done here
          pos++; //start reading after the "."
        }
      }
      pos = 0;
      if (server.argName(i) == "GW")
      {
        for (j = 0; j < 4; j++ ) //a valid ip is in the form "192.168.1.2", look for this pattern
        {
          String number = server.arg(i).substring(pos);    // get characters (toInt ignores non integer values)
          config.Gateway[j] = (uint8_t)(number.toInt() & 0xFF);
          pos = server.arg(i).indexOf(".", pos);
          if (pos < 0) break; //no dot found, this is an invalid address or we are done here
          pos++; //start reading after the "."
        }
      }
      if (server.argName(i) == "API_KEY")  config.APIkey = server.arg(i);
      if (server.argName(i) == "AP_NAME") config.DeviceName = urldecode(server.arg(i));
      if (server.argName(i) == "AP_PASS") config.DevicePW = urldecode(server.arg(i));
      if (server.argName(i) == "S_URI") config.serverURI = urldecode(server.arg(i)); //server uri
      if (server.argName(i) == "S_ADD") config.serveraddress = urldecode(server.arg(i)); //server address
      if (server.argName(i) == "S_PORT") config.serverport = server.arg(i).toInt();  //server port


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
  pageContent += " Use DHCP<BR><BR>\n";

  if (!config.useDHCP)
  {
    pageContent += "<b>Use fixd IP:</b><BR>\n";

    pageContent += "<b>";
    pageContent += "</b> <input type=\"text\" size=\"15\" maxlength=\"15\" name=\"IP\" value=\"";
    pageContent += String(config.IP[0]);
    pageContent += ".";
    pageContent += String(config.IP[1]);
    pageContent += ".";
    pageContent += String(config.IP[2]);
    pageContent += ".";
    pageContent += String(config.IP[3]);
    pageContent += "\"> IP<BR>\n";

    pageContent += "<b>";
    pageContent += "</b> <input type=\"text\" size=\"15\" maxlength=\"15\" name=\"NM\" value=\"";
    pageContent += String(config.Netmask[0]);
    pageContent += ".";
    pageContent += String(config.Netmask[1]);
    pageContent += ".";
    pageContent += String(config.Netmask[2]);
    pageContent += ".";
    pageContent += String(config.Netmask[3]);
    pageContent += "\"> Netmask<BR>\n";

    pageContent += "<b>";
    pageContent += "</b> <input type=\"text\" size=\"15\" maxlength=\"15\" name=\"GW\" value=\"";
    pageContent += String(config.Gateway[0]);
    pageContent += ".";
    pageContent += String(config.Gateway[1]);
    pageContent += ".";
    pageContent += String(config.Gateway[2]);
    pageContent += ".";
    pageContent += String(config.Gateway[3]);
    pageContent += "\"> Gateway<BR>\n";
  }

  pageContent += "<HR>"; //horizontal line-----------------------------------------------------------------------

  // pageContent += "<form action=\"\" method=\"GET\">\n";
  pageContent += "<b>Mutatio Configuration</b><BR><BR>\n";


  pageContent += "<input type=\"text\" size=\"28\" maxlength=\"31\" name=\"S_ADD\" value=\"";
  pageContent += String(config.serveraddress);
  pageContent += "\">:";
  pageContent += "<input type=\"text\" size=\"4\" maxlength=\"4\" name=\"S_PORT\" value=\"";
  pageContent += String(config.serverport);
  pageContent += "\">  Server address:port\n <BR>\n";
  pageContent += "<input type=\"text\" size=\"28\" maxlength=\"30\" name=\"S_URI\" value=\"";
  pageContent += config.serverURI;
  pageContent += "\"> Server URI<BR>\n";

  pageContent += "<input type=\"text\" size=\"28\" maxlength=\"16\" name=\"API_KEY\" value=\"";
  pageContent += config.APIkey;
  pageContent += "\"> Server API Key <BR>\n";

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

  if (config.useSDcard && SD_initialized)
  {
    pageContent += "<a href = \"SD.htm\">Show SD content</a>";
  }
  //todo: add status of node here (SDcard, RTC, Signal)

  server.send ( 200, "text/html", pageContent );
}


void sendSDPage() //send contents of root directory (no subdirectories supported)
{
  String pageContent = "<HTML><HEAD> <title>Mutatio SD content</title></HEAD>\n";
  pageContent += "<BODY bgcolor=\"#00ccff\" text=\"#000000\">";
  pageContent += "<FONT size=\"6\" FACE=\"Verdana\">\n<BR><b>SD content</b><BR><BR></font>";

  File dir = SD.open("/");
  dir.rewindDirectory();
  while (true) {
    File entry =  dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    if (entry.isDirectory()) {
      continue; //skip subdirectories
    }
    pageContent += "<a href=\"";
    pageContent += entry.name();
    pageContent += "\">";
    pageContent += entry.name();
    pageContent += "</a>";
    pageContent += "&emsp;&emsp;";
    pageContent += entry.size();
    pageContent += "B<BR>";
    entry.close();
  }
  dir.close();
  pageContent += "<HR>"; //horizontal line-----------------------------------------------------------------------

  server.send ( 200, "text / html", pageContent );

}


String getContentType(String filename) {
  if (server.hasArg("download")) return "application / octet - stream";
  else if (filename.endsWith(".htm")) return "text / html";
  else if (filename.endsWith(".html")) return "text / html";
  else if (filename.endsWith(".css")) return "text / css";
  else if (filename.endsWith(".js")) return "application / javascript";
  else if (filename.endsWith(".png")) return "image / png";
  else if (filename.endsWith(".gif")) return "image / gif";
  else if (filename.endsWith(".jpg")) return "image / jpeg";
  else if (filename.endsWith(".ico")) return "image / x - icon";
  else if (filename.endsWith(".xml")) return "text / xml";
  else if (filename.endsWith(".pdf")) return "application / x - pdf";
  else if (filename.endsWith(".zip")) return "application / x - zip";
  else if (filename.endsWith(".gz")) return "application / x - gzip";
  return "text / plain";
}

bool loadFromSdCard(String path) {
  File dataFile = SD.open(path.c_str());
  if (dataFile.isDirectory()) {
    path += " / index.htm";
    dataFile.close();
    dataFile = SD.open(path.c_str());
  }
  if (!dataFile)
  {
    path += ".gz"; //try again with gzipped version
    dataFile = SD.open(path.c_str());
  }
  if (!dataFile)
  {
    Serial.println("File Not Found: " + path);
    return false;
  }
  String dataType = getContentType(path);
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    Serial.println(F("Sent less data than expected!"));
  }
  dataFile.close();
  Serial.println("File sent: " + path);
  return true;
}

void handleNotFound() {
  if (config.useSDcard && SD_initialized)
  {
    if (loadFromSdCard(server.uri())) return;
  }
  //file not found on SD, return 404
  String message = "404 Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME: " + server.argName(i) + "\n VALUE: " + server.arg(i) + "\n";
  }
  server.send(404, "text / plain", message);
  Serial.println(message);
}

