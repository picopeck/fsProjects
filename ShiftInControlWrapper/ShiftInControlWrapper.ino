/*
Tester for ShiftInControl

07/06/16 - reduced for ShiftIncontrol only

*/

#include "ShiftInControl.h"
#include "max7219Control.h"


const int POLL_DELAY_MSEC = 200;

char* radioTags[] = {"NONE", "COMM1", "COMM2", "NAV1", "NAV2", "ADF", "XPDR"};

//enum eRSCONTROL {NAV2_SELECTED, a, NAV1_SELECTED, b, RS_FREQUENCY_DOWN, RS_FREQUENCY_UP, RS_TEST_BUTTON, RS_TRANSFER_BUTTON, COMM2_SELECTED, RS_MHz_BUTTON, d, e, f, COMM1_SELECTED, XPDR_SELECTED, ADF_SELECTED};
enum eRSCONTROL {RS_TEST_BUTTON, RS_TRANSFER_BUTTON, RS_FREQUENCY_UP, RS_FREQUENCY_DOWN, COMM1_SELECTED, COMM2_SELECTED, NAV1_SELECTED, NAV2_SELECTED, ADF_SELECTED, XPDR_SELECTED, RS_MHz_BUTTON};
int radioSelectorSwitch[] = {COMM1_SELECTED, COMM2_SELECTED, NAV1_SELECTED, NAV2_SELECTED, ADF_SELECTED, XPDR_SELECTED};
#define RADIOSELECTORNUMCONTACTS 6


// ### Pin allocations ###
int max7219DataIn = 13; //DIN
int max7219CLK = 12; //CLK
int max7219LOAD = 11; //CS
#define NUMBER_OF_max7219_DEVICES 3

PImax7219Control max7219Control = PImax7219Control(max7219DataIn, max7219CLK, max7219LOAD, NUMBER_OF_max7219_DEVICES);

/*
  initialise parameters for switch inputs
*/
int pLoadPin74HC165 = 7; //PL on chip, pin 1
int clockEnablePin74HC165 = 9; //CE on chip, pin 15
int dataPin74HC165 = 6; // Q7 on chip, pin 9
int clockPin74HC165 = 8; //CP on chip, pin 2
#define NUMBER_OF_SHIFT_REGISTERS_74HC165 2 //although can handle up to 8, the return 'value' is limited to a long i.e. 32 (4 devices)

PI74HC165Control autoPilotSwitches = PI74HC165Control(pLoadPin74HC165, clockEnablePin74HC165, dataPin74HC165, clockPin74HC165, NUMBER_OF_SHIFT_REGISTERS_74HC165);

int timer=0;
void setup()
{
  Serial.begin(115200); //open serial port for communications
}

void loop()
{
//Serial.println(timer);
    if (autoPilotSwitches.readState()!=autoPilotSwitches.previousState())
    {
      autoPilotSwitches.printState();
    

    autoPilotSwitches.update();//sets the previous switch states to the current ones.
    }
  delay(POLL_DELAY_MSEC);
//Serial.println(autoPilotSwitches.readState());
timer++;
}
