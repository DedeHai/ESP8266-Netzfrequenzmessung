
unsigned long fibonacci_ = 1; //fibonacci folge, wird erzeugt für immer längere delays.

#define TRACETOKEN "a64me5b8da"
#define PLOTLYAPIKEY "zpqjn0imzi"
#define PLOTLYUSER "damianOpto"


void plotly_openStream() {

  Serial.println(F("... Connecting to plotly's streaming servers..."));
  uint8_t errorcounter = 0;
  while (!client.connect("arduino.plot.ly", 80)  ) {
    fibonacci_ += fibonacci_;
    delay(fibonacci_);
    errorcounter++;
    if (errorcounter > 200)
    {
      Serial.println(F("FAIL"));
      return; //errorcounter is incremented in connect function
    }
  }
  fibonacci_ = 1;


  String header = "POST / HTTP/1.1\r\n";
  header += "Host: arduino.plot.ly\r\n";
  header += "User-Agent: Arduino\r\n";
  header += "Transfer-Encoding: chunked\r\n";
  header += "Connection: keep-alive\r\n";
  header += "\r\n";
  client.print(header);
  Serial.println(F("... Done initializing, ready to stream!"));
}



uint8_t plotly_init(bool createnewfile) { //if createnewfile is true, the file is created or overwritten, data is appended otherwise

  bool plotlyOK = false;
  Serial.println(F("Initializing plot.ly "));
  uint8_t errorcounter = 0;
  //open connection to plotly server
  while (!client.connect("plot.ly", 80)  ) {
    fibonacci_ += fibonacci_;
    delay(fibonacci_);
    errorcounter++;
    if (errorcounter > 200) return -1; //errorcounter is incremented in connect function
  }
  fibonacci_ = 1;

  String datastr = "un=";
  datastr += PLOTLYUSER;
  datastr += "&key=";
  datastr += PLOTLYAPIKEY;
  datastr += "&origin=plot&platform=arduino&version=2.3";
  datastr += "&args=[{\"x\": [], \"y\": [], \"name\": \"Frequenzabweichung\"";
  datastr += ", \"type\": \"scatter\", \"stream\": {\"token\": \"" ;
  datastr += TRACETOKEN ;
  datastr += "\", \"maxpoints\": 10000}}]";
  datastr += "&kwargs={\"filename\": \"Netz_ElHierro\",\"fileopt\":";
  if (createnewfile)
  {
    datastr += "\"overwrite\"";  //extend or overwrite for fileopt
  }
  else
  {
    datastr += "\"extend\"";  //extend or overwrite for fileopt
  }
  datastr += ", \"world_readable\": true}\r\n" ;


  //  String datastr = "un=";
  //  datastr += PLOTLYUSER;
  //  datastr += "&key=";
  //  datastr += PLOTLYAPIKEY;
  //  datastr += "&origin=plot&platform=lisp";
  //  datastr += "&args=[[0, 1, 2], [3, 4, 5], [1, 2, 3], [6, 6, 5]]";
  //  datastr += "&kwargs={\"filename\": \"Netzfrequenz\",\"fileopt\": \"overwrite\", \"world_readable\": true}\r\n" ;

  //&kwargs={"filename": "Temperature and Pressure", "fileopt": "overwrite", "style": {"type": "scatter"}, "layout": {"title": "Temperature and Pressure vs. Time", "xaxis": {"title": "Time (s)"}, "yaxis": {"title": "Temperature (Celsius)"}, "yaxis2": {"title": "Pressure"}}, "world_readable": true}

  String header = "POST /clientresp HTTP/1.1\r\n";
  header += "Host: plot.ly\r\n";
  header += "User-Agent: Arduino/0.6.0\r\n";
  header += "Content-Type: application/x-www-form-urlencoded\r\n";
  header += "Content-Length: ";
  header += String(datastr.length());
  header += "\r\n\r\n";
  client.print(header);
  client.print(datastr);

  Serial.print(header);
  Serial.print(datastr);
  uint8_t timeout = 0;
  while (client.available() == 0)
  {
    delay(10);
    if (timeout++ > 200) break; //wait 2s max
  }
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (line.indexOf("All Streams Go!") != -1)
    {
      Serial.println("OK from plotly");
      plotlyOK = true;
    }
    Serial.print(line); //print out what we received
  }

  client.stop(); //done initializing, time to open the stream now



  if ( plotlyOK == false)
  {
    Serial.println("FAIL");
    return -1;
  }
  else
  {
    plotly_openStream(); //open the stream to plotly
    return 0;
  }
}



void plotly_plot(Measurement datapoint)
{

  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line); //print out what we received
  }

  if (client.connected())
  {
   // Serial.println("plotly connected");

  }
  else
  {
   // Serial.println("disconnected");
     plotly_openStream(); //open the stream to plotly
  }



  //plot the data
  // String xdata = String(datapoint.Timestamp - 2208988800UL) + "." + String(datapoint.milliseconds);
  //plotly format is: "yyyy-mm-dd HH:MM:SS", 19 bytes long

  RtcDateTime timeelements;
  timeelements.InitWithEpoch32Time(datapoint.Timestamp - 2208988800UL); //converte epoch to date, month etc.
  char temparr[5];
  sprintf(temparr, "%02u", timeelements.Month()); //need a fixed length, easiest using sprintf
  String monthstr = String(temparr);
  sprintf(temparr, "%02u", timeelements.Day()); //need a fixed length, easiest using sprintf
  String daystr = String(temparr);
  sprintf(temparr, "%02u", timeelements.Hour() + 1); //need a fixed length, easiest using sprintf
  String hourstr = String(temparr);
  sprintf(temparr, "%02u", timeelements.Minute()); //need a fixed length, easiest using sprintf
  String minutestr = String(temparr);
  sprintf(temparr, "%02u", timeelements.Second()); //need a fixed length, easiest using sprintf
  String secondstr = String(temparr);

  String xdata = "\"" + String(timeelements.Year()) + "-" +  monthstr + "-" +  daystr + " " +  hourstr + ":" +  minutestr + ":" +  secondstr + "\"" ;


  String ydata = String(((float)datapoint.data / 100000) + 50, 5);
  client.print(xdata.length() + ydata.length() + 44, HEX);
  String senddata = "\r\n{\"x\": " + xdata + ", \"y\": " + ydata + ", \"streamtoken\": \"" + TRACETOKEN + "\"}\n\r\n";
  client.print(senddata);
 // Serial.println(senddata);



  // client.print("0\r\n\r\n"); //close the stream
  // client.stop();

}















