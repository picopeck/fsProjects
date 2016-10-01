/*
  PeckIndustries
  For interfacing a switch to an analog arduino input.
  v01/10/16 - first draft
*/

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
  val = analogRead(analogPin);    // read the input pin
  //if (val != preval)
  //{
    Serial.println(val);             // debug value
  //  preval = val;
  //}

}

