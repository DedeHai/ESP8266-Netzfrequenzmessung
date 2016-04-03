/*
 
  31.1.2016
  -added changelog file
  -improved NTP time tracking: more robust to time errors: if deviation is too big, do a fastupdate
  -added SD log entry for cpu frequency
  -changed SD logfile naming to suit 3 digit days (i.e. 2016031)
  
  1.2.2016
  -changed some timing constant in NTP_Time 
  -Removed saving unsent data if not requested by configuration
  -Bug in writeMeasurement() function corrected: >=1000 -> add 1 second (was >1000 -> add 1 second)
  
  2.2.2016
  -made main loop faster in case wifi signal is lost (changed delay)
  -fixed bug: LED not being updated when signal is not available
  
  3.2.2016
  -fixed bug in FCPU error eeprom writeback condition
  
  7.2.2016
  -commented out all the thingspeak test stuff 
  -added sendout server configuration handling to functions and eeprom (address, port and URI)
  -handle config.sendAllData correctly
  -add non-DHCP control to web page and to wifi handling
  -fixed nasty bug of complete lockout when invalid IP is assigned when not using DHCP (fixed by starting accesspoing if NTP server access fails 10 times in a row)
  -fixed bug in RTC functions to not make RTCTimeValid = true if RTC is not actually connected and config.useRTC is set
 
 9.2.2016
 -fixed bug in LED blink function to not blink blue if SD is not present
 -fixed inaccuracy bug in FCPU measurement when offset is high

 19.2.2016
 -improved plot.ly capability to detect disconnection and added proper timestamps

 21.2.2016
 -add SD readout to webpage, SD card can now be accessed over WiFi

 11.3.2016
 -bugfix: overflow in measurement when deviation is >320mHz
 -merged back with elhierro branch and added a define for a more dynamic island grid

 3.4.2016
 -added support for REV2 board: just define REV2 in the main file
 */
