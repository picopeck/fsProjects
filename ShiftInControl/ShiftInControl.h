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
    int readPin(int fIndex);
    void printState();
    unsigned long readState();
    unsigned long previousState();

  private:
    unsigned long _curValues;
    unsigned long _oldValues;
    int PULSE_WIDTH_USEC;
    unsigned int DATA_WIDTH_74HC165;
    int _numDevices;
    int _loadPin;
    int _clockEnablePin;
    int _clockPin;
    int _dataInPin;
};

#endif
