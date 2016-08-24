/*
a control can have the following properties...
toggle switch/momentary push button - On pin
multiple position switch - array of pins
encoder - CW, CCW pins

assumption is index 0 = CW or On pin or 1st pin of multi-switch
index 1 = CCW or 2nd pin of multi-switch
index 2 = 3rd pin of multip-switch etc

only pass the required number.

v2.0.0 - Accepts an array of indexes which describe where the switches are within the status
Could include the HC565 control directly in here??
v1.0.0 - Expects individual 'control' to be setup.
*/

#include "Controls.h"
#include <Arduino.h>
#include <Print.h>

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

PIControls::PIControls(int fNumSwitches)
{
	Serial.println("init Controls...");
	_numSwitches = fNumSwitches;

	setState(0); //sets the current state to zero.  just for initialising, they'll soon get overridden by programmatics.
	update(); //copies the current to the last
}

void PIControls::update()
{
	for (int i = 0; i < _numSwitches; i++)
	{
		_lastState[i] = _currentState[i];
	}
}

void PIControls::setState(unsigned long fValue)
{
	//should be preceeded by a 74HC165Control.readState();
	//gets the state of the switch from the input

	for (int i = 0; i < _numSwitches; i++)
	{
		_currentState[i] = bitRead(fValue, i);
	}
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

could i define an 'encoder' class which would handle all the encoder related info?
i.e. encoderClass altitudeEncoder(int fCW, int fCCW);
so the class would have public parameters such as last and current position
calling program would do something like...

getPinValues() //which would set the status of them all
then for each encoder/switch/momentary
altitudeEncoder.setStatus(readBit(pinValues74HC165, AP_ALTITUDE_UP),readBit(pinValues74HC165, AP_ALTITUDE_DOWN));
if (altitudeEncoder.isClockwise()==true) then Serial.println("B03");
if (altitudeEncoder.isCounterClockwise()==true) then Serial.println("B05");
altitudeEncoder.PIupdate(); //this function could be called to set the current values to the previous values. i.e. after processing by the main calling function

*/

bool PIControls::isClockwise(int pinIndex1, int pinIndex2)
{
	bool bIsClockwise = false;

	if ((_lastState[pinIndex1] == 0) && (_lastState[pinIndex2] == 0)) //phase 1 (CW), phase 4 (CCW)
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentState[pinIndex1] == 0) && (_currentState[pinIndex2] == 1)) //phase 2 (CW)
		{
			bIsClockwise = true;
		}
		//else {bIsClockwise=false;} // all other combinations are invalid
	}

	if ((_lastState[pinIndex1] == 0) && (_lastState[pinIndex2] == 1)) //phase 2 (CW), phase 3 (CCW)
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentState[pinIndex1] == 1) && (_currentState[pinIndex2] == 1)) //phase 3 (CW)
		{
			bIsClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}

	if ((_lastState[pinIndex1] == 1) && (_lastState[pinIndex2] == 1)) //phase 3 (CW), phase 2 (CCW)
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentState[pinIndex1] == 1) && (_currentState[pinIndex2] == 0)) //phase 4 (CW)
		{
			bIsClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}

	if ((_lastState[pinIndex1] == 1) && (_lastState[pinIndex2] == 0)) //phase 4 (CW), phase 1 (CCW)
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentState[pinIndex1] == 0) && (_currentState[pinIndex2] == 0)) //phase 1 (CW)
		{
			bIsClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}
	return (bIsClockwise);
}

bool PIControls::isCounterClockwise(int pinIndex1, int pinIndex2)
{
	bool bIsCounterClockwise = false;

	if ((_lastState[pinIndex1] == 0) && (_lastState[pinIndex2] == 0))
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentState[pinIndex1] == 1) && (_currentState[pinIndex2] == 0))
		{
			bIsCounterClockwise = true;
		}
		//else {bIsCounterClockwise=false;} // all other combinations are invalid
	}

	if ((_lastState[pinIndex1] == 0) && (_lastState[pinIndex2] == 1))
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentState[pinIndex1] == 0) && (_currentState[pinIndex2] == 0))
		{
			bIsCounterClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}

	if ((_lastState[pinIndex1] == 1) && (_lastState[pinIndex2] == 1))
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentState[pinIndex1] == 0) && (_currentState[pinIndex2] == 1))
		{
			bIsCounterClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}

	if ((_lastState[pinIndex1] == 1) && (_lastState[pinIndex2] == 0))
	{
		//check to see what current state is, therefore can determine the movement direction
		if ((_currentState[pinIndex1] == 1) && (_currentState[pinIndex2] == 1))
		{
			bIsCounterClockwise = true;
		}
		//else {lclDirection=UNDETERMINED;} // all other combinations are invalid
	}

	return (bIsCounterClockwise);
}

int PIControls::encoderDirection()
{
	int lclDirection = 0;
	if (isClockwise()) lclDirection = INCREMENT;
	if (isCounterClockwise()) lclDirection = DECREMENT;

	return (lclDirection);
}

/*
returns whether the indexed pin has gone from low to high.
*/

bool PIControls::isLOW2HIGH(int pinIndex)
{
	return (_lastState[pinIndex] < _currentState[pinIndex]);
}

/*
returns whether the indexed pin has gone from low to high.
*/
bool PIControls::isHIGH2LOW(int pinIndex)
{
	return (_lastState[pinIndex] > _currentState[pinIndex]);
}


/*
can receive an index input to see whether a particular switch position of multi-switch is on.
by default it will return the state of index 0
used for multi-switches i.e. PIControls.isHIGH(switchPos3) will determine whether the 3rd position on the switch indexing is true (active) or not
*/
bool PIControls::isHIGH(int pinIndex)
{
	return (_currentState[pinIndex] == 1);
}

bool PIControls::isLOW(int pinIndex)
{
	return (_currentState[pinIndex] == 0);
}

/*
returns the direction that the switch has turned.  Used for momentary or On/Off switches
fIndex is the bit within the 'pinValues74HC165' which equates to the Switch input

Values are 0->1 Switch state changed
1->0 Switch state changed
1->1 Switch state not changed but still HIGH
0->0 Switch state not changed but still LOW
*/
int PIControls::transitionDirection()
{
	int lclDirection = 0;

	if (isLOW2HIGH()) lclDirection = LOW2HIGH;
	if (isHIGH2LOW()) lclDirection = HIGH2LOW;

	return (lclDirection);
}
