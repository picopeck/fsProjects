#ifndef PIControls_h
#define PIControls_h

class PIControls
{
private:
	int _indexCW;
	int _indexCCW;
	int _currentState[];
	int _lastState[];
	int _pinIndexes[];
	int _numSwitches;

public:
	PIControls(int fNumSwitches);
	bool isClockwise(int pinIndex1 = 0, int pinIndex2 = 1);
	bool isCounterClockwise(int pinIndex1 = 0, int pinIndex2 = 1);
	int encoderDirection();
	void update();
	void setState(unsigned long fValue);
	bool isLOW2HIGH(int pinIndex = 0);
	bool isHIGH2LOW(int pinIndex = 0);
	bool isHIGH(int pinIndex);
	bool isLOW(int pinIndex);
	int transitionDirection();
#define INCREMENT 1
#define DECREMENT -1
#define LOW2HIGH 2
#define HIGH2LOW -2

};

#endif
