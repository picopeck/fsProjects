/*
Control using the 74HC165 device.
Can handle up to 4 device (32 inputs).
The indexes run from 0 to 31 and can be called upon individually for the status

Usage from the calling program would be a .readState and then subsequent actions.
functions like isHIGH(1) and isLOW(0) will return the status of a given index

Use as a rotary encoder...
Use the isCLOCKWISE or isCOUNTERCLOCKWISE to determine which direction the encoder is turning.
the CW index is the pin on which the clockwise terminal is wired, and CCW the anticlockwise terminal.
*/

#include "ShiftInControl.h"
#include <Arduino.h>

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

PI74HC165Control::PI74HC165Control(int fLoadPin, int fClockEnable, int fDataIn, int fClockPin, int fNumDevices)
{
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
	_numDevices = fNumDevices;
	if (fNumDevices > 8) _numDevices = 8;
	PULSE_WIDTH_USEC = 5;

	DATA_WIDTH_74HC165 = _numDevices * 8;

}

/*
used to updatte the oldvalues with the latest read values.  To be called after a .readState such that differences can be known for encoder purposes etc.
*/
void PI74HC165Control::update()
{
	_oldValues = _curValues;
}

/*
Privater function to read the status of a given index in the array
*/
int PI74HC165Control::readPin(int fIndex, bool lastValue)
{
	int bitValue = 0;
	if (!lastValue)
	{
		bitValue = bitRead(_curValues, fIndex);
	}
	else
	{
		bitValue = bitRead(_oldValues, fIndex);
	}
	return (bitValue);
}
/*
used to print the status of the indexes for debug purposes
*/
void PI74HC165Control::printState()
{
	Serial.println("Pin States:");
	Serial.println(DATA_WIDTH_74HC165);
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
/*
reads the state of the devices
*/
unsigned long PI74HC165Control::readState()
{
	unsigned long bitVal;
	unsigned long bytesVal = 0;
	//Serial.println("FUNCTION...readState");
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
	_curValues = bytesVal; //reads the button states	
	return (bytesVal);
	//Serial.println("...End.");
}
/*
returns the previously stored state.  Would need a .update prior to ensure its current
*/
unsigned long PI74HC165Control::previousState()
{
	return (_oldValues);
}

/*
returns whether the encoder is turning clockwise
curCW is the current state of the clockwise pin
lastCW is the previous state of the clockwise pin
curCCW is the current state of the counterclockwise pin
lastCCW is the previous state of the counterclockwise pin


|----|    |----|
CW    ----|    |----|    |----

|----|    |----|    |--
CCW   --|    |----|    |----|

CW
phase	|	CW	|	CCW
1   |	0	|	0
2   |	0	|	1
3   |	1	|	1
4   |	1	|	0

CCW
phase	|	CW	|	CCW
1   |	1	|	0
2   |	1	|	1
3   |	0	|	1
4   |	0	|	0

*/

bool PI74HC165Control::isClockwise(int pinIndex1, int pinIndex2)
{
	bool bIsClockwise = false;
	_lastStateCW = readPin(pinIndex1, true);
	_lastStateCCW = readPin(pinIndex2, true);
	_currentStateCW = readPin(pinIndex1);
	_currentStateCCW = readPin(pinIndex2);

	if ((_lastStateCW == 0) && (_lastStateCCW == 0)) //phase 1 (CW), phase 4 (CCW)
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentStateCW == 0) && (_currentStateCCW == 1)) //phase 2 (CW)
		{
			bIsClockwise = true;
		}
		//else {bIsClockwise=false;} // all other combinations are invalid
	}

	if ((_lastStateCW == 0) && (_lastStateCCW == 1)) //phase 2 (CW), phase 3 (CCW)
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentStateCW == 1) && (_currentStateCCW == 1)) //phase 3 (CW)
		{
			bIsClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}

	if ((_lastStateCW == 1) && (_lastStateCCW == 1)) //phase 3 (CW), phase 2 (CCW)
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentStateCW == 1) && (_currentStateCCW == 0)) //phase 4 (CW)
		{
			bIsClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}

	if ((_lastStateCW == 1) && (_lastStateCCW == 0)) //phase 4 (CW), phase 1 (CCW)
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentStateCW == 0) && (_currentStateCCW == 0)) //phase 1 (CW)
		{
			bIsClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}
	return (bIsClockwise);
}

bool PI74HC165Control::isCounterClockwise(int pinIndex1, int pinIndex2)
{
	bool bIsCounterClockwise = false;
	_lastStateCW = readPin(pinIndex1, true);
	_lastStateCCW = readPin(pinIndex2, true);
	_currentStateCW = readPin(pinIndex1);
	_currentStateCCW = readPin(pinIndex2);

	if ((_lastStateCW == 0) && (_lastStateCCW == 0))
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentStateCW == 1) && (_currentStateCCW == 0))
		{
			bIsCounterClockwise = true;
		}
		//else {bIsCounterClockwise=false;} // all other combinations are invalid
	}

	if ((_lastStateCW == 0) && (_lastStateCCW == 1))
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentStateCW == 0) && (_currentStateCCW == 0))
		{
			bIsCounterClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}

	if ((_lastStateCW == 1) && (_lastStateCCW == 1))
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentStateCW == 0) && (_currentStateCCW == 1))
		{
			bIsCounterClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}

	if ((_lastStateCW == 1) && (_lastStateCCW == 0))
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentStateCW == 1) && (_currentStateCCW == 1))
		{
			bIsCounterClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}

	return (bIsCounterClockwise);
}
/*
returns the encoderDirection as an integer
*/
int PI74HC165Control::encoderDirection(int pinIndexCW, int pinIndexCCW)
{
	int lclDirection = 0;
	if (isClockwise(pinIndexCW, pinIndexCCW)) lclDirection = INCREMENT;
	if (isCounterClockwise(pinIndexCW, pinIndexCCW)) lclDirection = DECREMENT;

	return (lclDirection);
}

/*
returns whether the indexed pin has gone from low to high.
*/

bool PI74HC165Control::isLOW2HIGH(int pinIndex)
{
	return (readPin(pinIndex, true) < readPin(pinIndex));
}

/*
returns whether the indexed pin has gone from low to high.
*/
bool PI74HC165Control::isHIGH2LOW(int pinIndex)
{
	return (readPin(pinIndex, true) > readPin(pinIndex));
}


/*
can receive an index input to see whether a particular switch position of multi-switch is on.
by default it will return the state of index 0
used for multi-switches i.e. PI74HC165Control.isHIGH(switchPos3) will determine whether the 3rd position on the switch indexing is true (active) or not
*/
bool PI74HC165Control::isHIGH(int pinIndex)
{
	return (readPin(pinIndex) == 1);
}

bool PI74HC165Control::isLOW(int pinIndex)
{
	return (readPin(pinIndex) == 0);
}

/*
returns the direction that the switch has turned.  Used for momentary or On/Off switches
fIndex is the bit within the 'pinValues74HC165' which equates to the Switch input

Values are
0->1 Switch state changed
1->0 Switch state changed
1->1 Switch state not changed but still HIGH
0->0 Switch state not changed but still LOW
*/
int PI74HC165Control::transitionDirection()
{
	int lclDirection = 0;

	if (isLOW2HIGH()) lclDirection = LOW2HIGH;
	if (isHIGH2LOW()) lclDirection = HIGH2LOW;

	return (lclDirection);
}
