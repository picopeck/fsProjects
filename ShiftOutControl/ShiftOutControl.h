/*	
ShiftOut Control Class	
74HC565 Device	
*/	
	
	
#ifndef PI74HC565Control_h	
#define PI74HC565Control_h	
	
class PI74HC565Control	
{	
  public:	
    PI74HC565Control(int fLatchPin, int fClockPin, int fDataPin, int fNumDevices=1);	
    void setLed(int fIndex, int fValue);	
    void printState();	
    void update();	
    unsigned long readState();	
    void setMultipleLed(unsigned long fValue);	
    void test(unsigned long fValue);	
	
  private:	
    unsigned long _value;	
    int PULSE_WIDTH_USEC;	
    unsigned int DATA_WIDTH_74HC565;	
    int _numDevices;	
    int _latchPin;	
    int _clockPin;	
    int _dataPin;	
}	;
	
#endif	
