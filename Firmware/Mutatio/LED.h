#define PIXEL_PIN 15
struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
RGB LEDcolor;
Adafruit_NeoPixel LED = Adafruit_NeoPixel(1, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

unsigned long LEDtimestamp; //timestamp for updating RGB led

//update RGB LED from time to time
void updateLED(void)
{


  /*
    static uint8_t toggler = 0;

    if ((millis() - LEDtimestamp) > 30) //update led approximately every 30 ms
    {
    LEDtimestamp = millis();
    if (toggler == 0) LEDcolor.blue++;
    else LEDcolor.blue--;

    if (LEDcolor.blue > 50) {
      toggler = 1;  //note: force check on NRF24 module every once in a while, maybe an interrupt was missed

    }
    if (LEDcolor.blue < 2) toggler = 0;
    LED.setPixelColor(0, LED.Color(LEDcolor.red, LEDcolor.green, LEDcolor.blue));
  */

  //only update led if there is enough time left to the next expected pin interrupt
  //it surely takes less than 80us (probably only 25), that is 6400CPU clocks. make that 12000 to be sure.

  if ((lastcapture + 1600000) - ESP.getCycleCount() > 12000)
  {
    if ((millis() - LEDtimestamp) > 30) //update led approximately every 30 ms
    {
      LEDtimestamp = millis();
      LED.setPixelColor(0, LED.Color(LEDcolor.r, LEDcolor.g, LEDcolor.b));
      LED.show();


      //
      //  if(leadbreathe == 0)
      //  {
      //    LED.setPixelColor(0, LED.Color(LEDcolor.red, LEDcolor.green, LEDcolor.blue));
      //
      //    LED.show();
      //  }
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


