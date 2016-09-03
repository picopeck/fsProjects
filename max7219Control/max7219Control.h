#include <LedControl.h>


#ifndef PImax7219Control_h	
#define PImax7219Control_h	

class PImax7219Control
{
public:
	PImax7219Control(int dataPin, int clkPin, int csPin, int numDevices = 1);
	void testDisplay();
	void displayNumber(int fDeviceID, long fNumber, int fStartIndex = 1, int fLength = 1, int fPower = 0, int fNegative = 1, bool fLeadingZero=true,bool fDEBUG = false);

private:
	LedControl _deviceHandle;
	int DELAY_TIME_uS=1000;
	int _numDevices;
	void main();

};

#endif
