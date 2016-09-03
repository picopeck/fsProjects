/*
  ShiftIn Control Class
  74HC165 Device
  */

#ifndef PI74HC165Control_h
#define PI74HC165Control_h

class PI74HC165Control
{
public:
	PI74HC165Control(int fLoadPin, int fClockEnable, int fDataIn, int fClockPin, int fNumDevices = 1);
	void update();
	void printState();
	unsigned long readState();
	unsigned long previousState();
	bool isClockwise(int pinIndexCW, int pinIndexCCW);
	bool isCounterClockwise(int pinIndexCW, int pinIndexCCW);
	int encoderDirection(int pinIndexCW, int pinIndexCCW);
	bool isLOW2HIGH(int pinIndex = 0);
	bool isHIGH2LOW(int pinIndex = 0);
	bool isHIGH(int pinIndex);
	bool isLOW(int pinIndex);
	int transitionDirection();

#define INCREMENT 1
#define DECREMENT -1
#define LOW2HIGH 2
#define HIGH2LOW -2

private:
	int readPin(int fIndex, bool lastValue = false);

	unsigned long _curValues;
	unsigned long _oldValues;
	int PULSE_WIDTH_USEC;
	unsigned int DATA_WIDTH_74HC165;
	int _numDevices;
	int _loadPin;
	int _clockEnablePin;
	int _clockPin;
	int _dataInPin;
	int _currentStateCW;
	int _currentStateCCW;
	int _lastStateCW;
	int _lastStateCCW;
};

#endif
