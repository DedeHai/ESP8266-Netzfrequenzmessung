
struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
RGB LEDcolor;

uint32_t LEDtimestamp; //timestamp for updating RGB led
uint32_t blinktimestamp;
//update RGB LED from time to time
void updateLED(void)
{


  //only update led if there is enough time left to the next expected pin interrupt
  //assumption is that interrupts do not happen faster than 5ms or every 400'000 CPU ticks
  //with a good signal, the interrupt happens no faster than after 8ms
  //it takes up to 5000CPU clocks (62µs). make sure the LED timing does not interfere with the interrupt (it deactivates interrupts while clocking)

  if ((ESP.getCycleCount() - lastcapture) < 400000)
  {


    if ((millis() - LEDtimestamp) > 5) //update led no faster than every 5 ms
    {
      LEDtimestamp = millis();

      if (millis() - blinktimestamp  > 1500)
      {
        // WIFI connection is lost - blink yellow
        // Measurement signal is lost - blink red
        // SD card access failed - blink blue

        if (wifiWatchdog > 0 ) //blink yellow
        {
          LEDcolor.r = 150;
          LEDcolor.g = 150;
          LEDcolor.b = 0;
        }
        if (sdWatchdog > 0 ) //blink blue
        {
          LEDcolor.r = 0;
          LEDcolor.g = 0;
          LEDcolor.b = 150;
        }
        if (signalWatchdog > 800 ) //blink red
        {         
          LEDcolor.r = 220;
          LEDcolor.g = 0;
          LEDcolor.b = 0;
        }
        if (millis() - blinktimestamp  > 2000) blinktimestamp = millis();
      }
      
      if(APactive > 0)
      {
          LEDcolor.r = 200;
          LEDcolor.g = 10;
          LEDcolor.b = 200;
      }
      LED.setPixelColor(0, LED.Color(LEDcolor.r, LEDcolor.g, LEDcolor.b));


      LED.show();
    }
  }
}

//hsv to rgb using floats, is more accurate than using just ints
RGB hsv_to_rgb(float H, unsigned char S, unsigned char V)  //input values are from 0 to 255, hue can be a float
{

  float s = (float)S / 255.0; //auf 1 normieren
  float v = (float)V / 255.0; //auf 1 normieren
  RGB result;
  int i;
  float f, p, q, t;
  if (s == 0 ) //zero saturation, return gray level
  {
    result.r = round(255 * v);
    result.g = round(255 * v);
    result.b = round(255 * v);
    return result;
  }


  i = (int)((float)H / 42.5) % 6;
  f = (float)H / 42.5 - (int)((float)H / 42.5);
  p = v * ( 1.0 - s );
  q = v * ( 1.0 - (s * f));
  t = v * ( 1.0 - (s * ( 1.0 - f )));


  switch ( i )
  {
    case 0:
      result.r = round(255 * v);
      result.g = round(255 * t);
      result.b = round(255 * p);
      break;

    case 1:
      result.r = round(255 * q);
      result.g = round(255 * v);
      result.b = round(255 * p);
      break;

    case 2:
      result.r = round(255 * p);
      result.g = round(255 * v);
      result.b = round(255 * t);

      break;

    case 3:
      result.r = round(255 * p);
      result.g = round(255 * q);
      result.b = round(255 * v);
      break;

    case 4:
      result.r = round(255 * t);
      result.g = round(255 * p);
      result.b = round(255 * v);
      break;

    default:
      result.r = round(255 * v);
      result.g = round(255 * p);
      result.b = round(255 * q);
      break;

  }

  return result;

}


