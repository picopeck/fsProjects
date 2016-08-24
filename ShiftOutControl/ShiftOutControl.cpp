#include "ShiftOutControl.h"
#include <Arduino.h>

PI74HC565Control::PI74HC565Control(int fLatchPin, int fClockPin, int fDataPin, int fNumDevices)
{
	_latchPin = fLatchPin;
	_clockPin = fClockPin;
	_dataPin = fDataPin;
	_numDevices = fNumDevices;

	//LED outputs pins	
	pinMode(_latchPin, OUTPUT);
	pinMode(_clockPin, OUTPUT);
	pinMode(_dataPin, OUTPUT);

	DATA_WIDTH_74HC565 = _numDevices * 8;
	test(pow(2, 32) - 1);
}

void PI74HC565Control::test(unsigned long fValue)
{
	setMultipleLed(fValue);
	update();
	delay(1000);
	setMultipleLed(0);
	update();
}

/*
Outputs the current status to the register
*/
void PI74HC565Control::update()
{
	int tempByte; //storage for a single byte of data as can't write a word	

	tempByte = _value; //will be limited to the first byte	

	for (int iLoopVar = 0; iLoopVar < _numDevices; iLoopVar++)
	{
		digitalWrite(_latchPin, LOW);
		shiftOut(_dataPin, _clockPin, MSBFIRST, tempByte);
		digitalWrite(_latchPin, HIGH);
		tempByte = _value >> (iLoopVar * 8); //shift the next 8 bytes in.  will be valid for all chips.	
		//was tempByte = tempByte >> (iLoopVar * 8); //shift the next 8 bytes in.  will be valid for all chips.	
	}
}

/*
Sets directly the status of the outputs
*/
void PI74HC565Control::setMultipleLed(unsigned long fValue)
{
	_value = fValue;
}


void PI74HC565Control::setLed(int fIndex, int fValue)
{
	bitWrite(_value, fIndex, fValue);
}

void PI74HC565Control::printState()
{
	unsigned long tempState = readState();
	/*  Serial.print("Pin States:\r\n");
	  for (int i = 0; i < DATA_WIDTH_74HC165; i++) //this compensates for the length of DATA_WIDTH_74HC165 i.e.
	  {
	  Serial.print(" Pin-");
	  Serial.print(i);
	  Serial.print(": ");

	  if ((_curValues >> i) & 1)
	  Serial.print("HIGH");
	  else
	  Serial.print("LOW");

	  Serial.print("\r\n");
	  }
	  Serial.print("\r\n");
	  */
}

unsigned long PI74HC565Control::readState()
{
}
