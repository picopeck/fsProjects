/*
Common Functions
*/

#include "PIFunctions.h"
#include <Arduino.h>

char PIFunctions::getChar()
{
	while (Serial.available() == 0);
	return ((char)Serial.read());
}

/*
  will return the integer of the number of 'numChars' characters, and multiply by iPower if its a decimal
  INPUTS
  int numChars - the number of characters to read for the value
  int iPower - the power to 10 which to multiply the number by to enable only long numbers
  */
int PIFunctions::getNumber(int numChars, int iPower)
{

	int cTempChar = 0;
	String lclStrTemp = "";
	float fTemp;
	for (int iLoopVar = 0; iLoopVar < numChars; iLoopVar++)
	{
		cTempChar = getChar();
		if (((cTempChar > 0) && (cTempChar < 10)) || (cTempChar == '.') || (cTempChar == '-'))
		{
			lclStrTemp += cTempChar;  //do i need to exit the loop if 'getChar' is an '=' or other qualifier i.e. not a number or a valid character such as '-'
		}
		else break;
	}
	fTemp = lclStrTemp.toFloat();
	fTemp = fTemp * pow(10, iPower);
	return ((int)fTemp);
}

int PIFunctions::getInt(int fNumChars, int fPower)
{
	//function to iterate the fNumChars number of characters and return the integer value i.e. for headings, altitude, vert speed

	String sTemp = "";
	//need to get next fNumChars characters
	for (int iLoopVar = 0; iLoopVar < fNumChars; iLoopVar++)
	{
		sTemp += getChar();
	}
	if (fPower > 0)
	{
		float temp;
		temp = (sTemp.toFloat());
		temp *= pow(10, fPower);
		sTemp = String(temp, 0);
	}
	return (sTemp.toInt());
}

long PIFunctions::getLong(int fNumChars, int fPower)
{
	//function to iterate the fNumChars number of characters and return the integer value i.e. for headings, altitude, vert speed

	String sTemp = "";
	//need to get next fNumChars characters
	for (int iLoopVar = 0; iLoopVar < fNumChars; iLoopVar++)
	{
		sTemp += getChar();
	}
if (fPower > 0)
	{
		float temp;
		temp = (sTemp.toFloat());
		temp *= pow(10, fPower);
		sTemp = String(temp, 0);
	}
	return (sTemp.toFloat());
}
