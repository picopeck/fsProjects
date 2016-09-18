/*
  Tester program for max7219Devices
*/

#include <LedControl.h>
#include <max7219Control.h>

//pin allocations
//7 segment output displays
int max7219DataIn = 13;
int max7219CLK = 12;
int max7219LOAD = 11;
#define NUMBER_OF_MAX7219_DEVICES 1

PImax7219Control max7219Control = PImax7219Control(max7219DataIn, max7219CLK, max7219LOAD, NUMBER_OF_MAX7219_DEVICES);

long val1 = 910;
long val2 = 799;


void setup()
{
  Serial.begin(115200); //open serial port for communications
  max7219Control.testDisplay();
}

void loop()
{
  //  Serial.println("hello");
  //displayNumber(int fDeviceID, long fNumber, int fStartIndex, int fLength, int fPower, int fNegative, bool fDEBUG)
  //max7219Control.displayNumber(0, val1, 1, 3);
    max7219Control.displayNumber(0, val2, 6, 3);
  if (val1 > 899)
  {
    //if speed greater than 899 then put a decimal and the last two digits
    // change max7219 to accept a flag for setting the leading zero or not for decimals.
    // void PImax7219Control::displayNumber(int fDeviceID, long fNumber, int fStartIndex, int fLength, int fPower, int fNegative, bool fLeadingZero, bool fDEBUG)

    max7219Control.displayNumber(0, (val1-900), 1, 3, 2, 1, false);  // the fPower forces the displayFunction to set the decimal point and remove the leading zero.
  }
  else
  {
    max7219Control.displayNumber(0, val1, 1, 3);
  }


  delay(500);

    val1-=1;
  
  val2 -= 1;
}

