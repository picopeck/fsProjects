/*
Common Functions
v10/10/16 - added the analogSwitchFunction
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


/*
  returns the switch position that the given analog input has detected
  int switchPosition : the index 0 to 7 of the available analog inputs on the Arduino UNO that has the required switch mounted.
  int numContacts : the number of contacts that the switch has.
  Usage : call the function and assign a variable to hold the returned switch position from 0 to 'numContacts'.  0 indicates an error.  switch positions are number clockwise, relative to the first position.  Assumption is that the switch can't go 360degs.
  +5V goes to pin 1 of switch
  GND goes to pin n of switch
  A0 goes to GND pin of switch
  */
int PIFunctions::analogSwitchPosition(int fIndex, int numContacts)
{
	//Serial.println("analogSwitchPositions...");
	int switchPosition = 0; //ERROR condition, it means it hasn't found a valid position.
	int slope = (1023 / (numContacts - 1));
	int intercept = slope * numContacts;
	int pinVoltage; //variable to store the sampled digitised voltage 0-1023 full scale.
	pinVoltage = analogRead(fIndex);
	//Serial.println(pinVoltage);
	// back calculate x to get the switch position.  negative slope as the slope is negative
	return  ((pinVoltage - intercept) / (-slope));
}
