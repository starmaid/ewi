//
//    FILE: HX710B_plotter.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: test basic behaviour
//     URL: https://github.com/RobTillaart/HX710AB


#include "HX710AB.h"

//  adjust pins to your setup
uint8_t dataPin = 6;
uint8_t clockPin = 7;

HX710B HX(dataPin, clockPin);

uint8_t dataPin1 = 8;
uint8_t clockPin1 = 9;

HX710B HX1(dataPin1, clockPin1);


void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println(__FILE__);
  Serial.print("HX710AB_LIB_VERSION: ");
  Serial.println(HX710AB_LIB_VERSION);
  Serial.println();

  HX.begin();
  HX1.begin();

  //  adjust to your raw measurements.
  HX.calibrate(50, 0, 100, 80);
  HX1.calibrate(50,0,100,80);
}

void loop()
{
  Serial.print("0,");
  Serial.print(HX.read(1));
  Serial.print(",");
  Serial.print(HX1.read(1));
  Serial.println();
  delay(10);
}


//  -- END OF FILE --
