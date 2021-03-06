/*
PIFunctions
*/

#ifndef PIFunctions_h
#define PIFunctions_h

class PIFunctions
{
public:
	char getChar();
	int getNumber(int numChars, int iPower=0);
	int getInt(int fNumChars, int fPower = 0);
	long getLong(int fNumChars, int fPower =0);
        int analogSwitchPosition(int fIndex, int numContacts);
};

#endif
