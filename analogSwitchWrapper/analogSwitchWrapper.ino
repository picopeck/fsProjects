/*
  PeckIndustries
  For interfacing a switch to an analog arduino input.
  v10/10/16 - modified pinout for rotary switch to be correct!
  v07/10/16 - the analogRead function has moved to PIFunctions so have added the header file for testing
  v01/10/16 - first draft
*/

#include "PIFunctions.h"

PIFunctions commonFunction;

int analogPin = 0;     // potentiometer wiper (middle terminal) connected to analog pin 3
// outside leads to ground and +5V
int val = 0;           // variable to store the value read
int preval = 0;
void setup()
{
  Serial.begin(115200);          //  setup serial
}

void loop()
{
  val = commonFunction.analogSwitchPosition(analogPin, 3);
  //val = analogRead(analogPin);    // read the input pin
  //if (val != preval)
  //{
    Serial.print("Switch Pos : ");
    Serial.println(val);
    delay(1000);
  //  preval = val;
  //}
}

