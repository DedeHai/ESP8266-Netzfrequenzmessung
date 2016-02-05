/*
 * 31.1.2016
 * -added changelog file
 * -improved NTP time tracking: more robust to time errors: if deviation is too big, do a fastupdate
 * -added SD log entry for cpu frequency
 * -changed SD logfile naming to suit 3 digit days (i.e. 2016031)
 * 
 * 1.2.2016
 * -changed some timing constant in NTP_Time 
 * -Removed saving unsent data if not requested by configuration
 * -Bug in writeMeasurement() function corrected: >=1000 -> add 1 second (was >1000 -> add 1 second)
 * 
 * 2.2.2016
 * -made main loop faster in case wifi signal is lost (changed delay)
 * -fixed bug: LED not being updated when signal is not available
 * 
 * 3.2.2016
 * -fixed bug in FCPU error eeprom writeback condition
 */
