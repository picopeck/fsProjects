/*
  PeckIndustries interface to flight sim for Autopilot module.
  Button and switch functions
  max7219 calls
  Common functions
  */

#include <LedControl.h>
#include "max7219Control.h"
#include <Arduino.h>
#include <Print.h>

PImax7219Control::PImax7219Control(int dataPin, int clkPin, int csPin, int numDevices) : _deviceHandle(dataPin, clkPin, csPin, numDevices)
{
	//can't run setup as the contructor hasnt finished yet!
}


void PImax7219Control::initDevices()
{
	//turn this into a 'turn all on' test function	
	//initialise the Max7219 devices	
	Serial.print("Device Init... ");
	_numDevices = _deviceHandle.getDeviceCount();
	Serial.println(_numDevices);
	//Serial.println(" display device(s)...");
	for (int address = 0; address < _numDevices; address++)
	{
		_deviceHandle.shutdown(address, false); 		//wake up the MAX72XX from power-saving mode
		_deviceHandle.setIntensity(address, 8);//set a medium brightness for the Leds
		_deviceHandle.clearDisplay(address);
		//sets the maximum number of characters to display.  need to set all unused characters to ' '	
		//_deviceHandle.setScanLimit(address, 8);
		/*
		for (int j = 0; j < 8; j++)
		{ //turns on all digits and decimal points
		_deviceHandle.setChar(address, j, '8', true);
		}
		delay(DELAY_TIME_uS);
		for (int j = 0; j < 8; j++)
		{ //turns on all digits and decimal points
		_deviceHandle.setChar(address, j, ' ', false);
		}*/
	}
	IBIT();
	Serial.println("Completed.");
	delay(DELAY_TIME_uS);
}

void PImax7219Control::IBIT()
{
	Serial.print("IBIT - Display Test Start...");
	for (int address = 0; address < _numDevices; address++)
	{
		for (int j = 0; j < 8; j++)
		{// turns on all digits and decimal points
			_deviceHandle.setChar(address, j, '8', true);
		}
		delay(DELAY_TIME_uS);
		for (int j = 0; j < 8; j++)
		{ // turn them all off again
			_deviceHandle.setChar(address, j, ' ', false);
		}
	}
	Serial.println("Completed.");
}

/* function for displaying a number on an 7 segment display
  INPUTS

  int fDeviceID - the number of the device (0-7) to write to of the iDeviceHandle
  long fNumber - the number to display.  If the number required is really a decimal, the value should be divided by 10^fPower accordingly.  Most notable in Mach speed.
  int fPower (optional) - the power of 10 with which to divide fNumber by to get decimal.
  int fNegative (optional) - tri-state, -1 if negative is to be displayed, 0 if no negative to be displayed, 1 if it shouldn't do anything.  Will display the '-' char at startindex-1
  int fStartindex - indexed from the left as 1st, 2nd etc.  7->0 for the display device.  remember indexing has 0 on the right of the display.
  int length - the length of 'number' in which to display.   i.e. could be startindex 5, length 3. will display indexs 0, 1, 2 of number starting at 5[3]
  bool fDEBUG (optional) - will display the debug data, Serial.prints

  -----------------
  |1|2|3|4|5|6|7|8|
  -----------------
  digitIndex  7 6 5 4 3 2 1 0
  12600             0 0 6 2 1
  numIndex

  */

void PImax7219Control::displayNumber(int fDeviceID, long fNumber, int fStartIndex, int fLength, int fPower, int fNegative, bool fLeadingZero, bool fDisplayDecimalAtZeroPower)
{
	int lclDigits[8]; //length of a max7219 device, each digit is stored in a single entry of the array	

	long lclNumber = fNumber;
	int lclStartIndex = fStartIndex;

	if ((lclStartIndex < 1) || (lclStartIndex>8))
	{
		Serial.println("####   ERROR   ####");
		Serial.println("fStartIndex cannot be '<1' or '>8'.");
		Serial.println("Display will default to position '1'.");
		Serial.println("####   ERROR   ####");
		lclStartIndex = 1;
	}

	/*
	  get each digit based upon the length
	  tenThousands, thousands, hundreds, tens, units
	  Assumes an integer number has been passed.  relies on pre-processing on fNumber
	  */
	//if (fDEBUG) Serial.println("lclDigits");
	for (int iLoopVar = 0; iLoopVar < 8; iLoopVar++)
	{
		lclDigits[iLoopVar] = ((int)(lclNumber % 10));
		lclNumber = lclNumber / 10;
		//_deviceHandle.setDigit(_numDevices-1, iLoopVar, (byte)lclNumber, false);
		//if (fDEBUG)
		//{
		//	Serial.print(iLoopVar);
		//	Serial.print(",");
		//	Serial.println(lclDigits[iLoopVar]);
		//}
	}

	int digitIndex = 0;
	int digitsArrayIndex = 0;
	int tempLength = fLength;

	if ((lclStartIndex + fLength)>9)
	{
		Serial.println("####   ERROR   ####");
		Serial.println("The length of fNumber exceeds the maximum characters.");
		Serial.println("fNumber will be trimmed.");
		Serial.println("####   ERROR   ####");
		tempLength = 9 - (lclStartIndex);
	}

	// don't clear any of the other digits currently written to the display...	
	for (int i = lclStartIndex; i < (lclStartIndex + tempLength); i++)
	{
		// example, start=6 length =3, any negative sign will get placed at startindex-1	
		digitIndex = (-i + 8); // index for digit within device, where 1 is the left most character	
		digitsArrayIndex = (((lclStartIndex + tempLength) - 1) - i);//index for number array
		// code for handling a decimal point.  the index at which to draw it is dependent upon startindex and the fPower value	
		if ((fPower>0) && (i == ((lclStartIndex + tempLength) - fPower - 1)))
		{
			if (fLeadingZero)
			{
				_deviceHandle.setDigit(fDeviceID, digitIndex, (byte)lclDigits[digitsArrayIndex], true);
			}
			else
			{
				_deviceHandle.setChar(fDeviceID, digitIndex, ' ', true);
			}  //no need for an ‘else’ as the previous setDigit calls will draw the appropriate number when fLeadingZero becomes true
		}
		else
		{
			if (!fDisplayDecimalAtZeroPower) _deviceHandle.setDigit(fDeviceID, digitIndex, (byte)lclDigits[digitsArrayIndex], false);
			else
			{
				if (i == (lclStartIndex + tempLength) - fPower - 1)
					_deviceHandle.setDigit(fDeviceID, digitIndex, (byte)lclDigits[digitsArrayIndex], true);
				else _deviceHandle.setDigit(fDeviceID, digitIndex, (byte)lclDigits[digitsArrayIndex], false);
			}
		}
	}

	//only want to do this if the device is for one that can be switched between negative or not.	
	if ((lclStartIndex == 1) && (fNegative < 1))
	{
		Serial.println("####   ERROR   ####");
		Serial.println("fStartIndex cannot be '1' for a negative number to be displayed.");
		Serial.println("####   ERROR   ####");
	}
	else
	{
		digitIndex = (-(lclStartIndex)+9);
		if (fNegative == -1) //put negative sign in (fStartIndex-1)th character position	
		{
			_deviceHandle.setChar(fDeviceID, digitIndex, '-', false);
		}
		else if (fNegative == 0)
		{
			_deviceHandle.setChar(fDeviceID, digitIndex, ' ', false);
		}
		else {}
	}

}

void PImax7219Control::blankDisplay(int fDeviceID)
{
	_deviceHandle.clearDisplay(fDeviceID);
}