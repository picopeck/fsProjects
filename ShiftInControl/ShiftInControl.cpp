#include "ShiftInControl.h"
#include <Arduino.h>

PI74HC165Control::PI74HC165Control(int fLoadPin, int fClockEnable, int fDataIn, int fClockPin, int fNumDevices)
{
	Serial.println("init ShiftIn...");
	_loadPin = fLoadPin;
	_clockEnablePin = fClockEnable;
	_clockPin = fClockPin;
	_dataInPin = fDataIn;
	pinMode(_loadPin, OUTPUT);
	pinMode(_clockEnablePin, OUTPUT);
	pinMode(_clockPin, OUTPUT);
	pinMode(_dataInPin, INPUT);
	digitalWrite(_clockPin, LOW);
	digitalWrite(_loadPin, HIGH);

	if (fNumDevices > 8) _numDevices = 8;
	PULSE_WIDTH_USEC = 5;

	DATA_WIDTH_74HC165 = _numDevices * 8;

	_curValues = readState(); //reads the button states	
	update();
}

void PI74HC165Control::update()
{
	_oldValues = _curValues;
}

int PI74HC165Control::readPin(int fIndex)
{
	bitRead(_curValues, fIndex);
}

void PI74HC165Control::printState()
{
	Serial.print("Pin States:\r\n");
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
}

unsigned long PI74HC165Control::readState()
{
	unsigned long bitVal;
	unsigned long bytesVal = 0;

	//trigger a parallel read to latch the current states	
	digitalWrite(_clockEnablePin, HIGH);
	digitalWrite(_loadPin, LOW);
	delayMicroseconds(PULSE_WIDTH_USEC);
	digitalWrite(_loadPin, HIGH);
	digitalWrite(_clockEnablePin, LOW);

	//loop to read each bit	
	for (int iLoopVar = 0; iLoopVar < DATA_WIDTH_74HC165; iLoopVar++)
	{
		bitVal = digitalRead(_dataInPin);

		bytesVal |= (bitVal << ((DATA_WIDTH_74HC165 - 1) - iLoopVar));

		//pulse the clock for next bit	
		digitalWrite(_clockPin, HIGH);
		delayMicroseconds(PULSE_WIDTH_USEC);
		digitalWrite(_clockPin, LOW);
	}
	return (bytesVal);
}

unsigned long PI74HC165Control::previousState()
{
	return (_oldValues);
}
