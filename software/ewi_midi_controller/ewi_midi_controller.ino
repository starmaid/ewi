// written by starmaid september 2025

#include <Adafruit_MPR121.h>
#include <Wire.h>
#include "HX710AB.h"

// Machine state
int DEVICE_MODE = 0; // 0 = pre-initialized, 1 = normal
int KEYS_HELD = 0b0000000000000000;
int NOTE_STATE = 0;

// Thresholds
int note_on_pressure_threshold = 0;
int button_pressed_cap_threshold = 0;

// define EWI fingerings
// write in binary which buttons are pressed and associate with a note.


// define sensor -> key map
// define a number in binary with one bit=1 for each button. if that button is held, it is added to the new KEYS_HELD value. that way we just do like 17 sums


// loop-local definitions
int new_keys_held = 0;


void setup()
{
  // set pins as input/output
  // start any communication steps
}

void loop()
{
  // set blank states
  
  // read from inputs

  // filter, pass thresholds

  // turn into logical values

  // change based on device state
  switch (DEVICE_MODE)
  {
  case 0:
    // calibrating...
    break;
  default:
    // normal mode, play notes etc.
    break;
  }
}
