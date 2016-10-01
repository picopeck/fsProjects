/*
Tester for ShiftInControl

07/06/16 - reduced for ShiftIncontrol only

*/

#include "ShiftInControl.h"


const int POLL_DELAY_MSEC = 1000;

enum eAPCONTROL
{
  AP_IASMACH_UP,
  AP_IASMACH_DOWN,
  AP_IASMACH_SWITCH,
  AP_ALTITUDE_UP,
  AP_ALTITUDE_DOWN,
  AP_ALTITUDE_HOLD,
  AP_HEADINGCOURSE_UP,
  AP_HEADINGCOURSE_DOWN,
  AP_HEADING_HOLD,
  AP_COURSE_HOLD,
  AP_HEADINGCOURSE_SWITCH,
  AP_IASMACH_HOLD,
  AP_VERTICALSPEED_UP,
  AP_VERTICALSPEED_DOWN,
  AP_VERTICALSPEED_HOLD,
  AP_AUTOPILOT_HOLD,
  AP_AUTOTHROTTLE_SWITCH,
  AP_FLIGHTDIRECTOR_SWITCH,
  AP_APPROACH_HOLD,
  AP_BACKCOURSE_HOLD,
  AP_NAV1_HOLD,
  AP_GPSDRIVESNAV1_HOLD,
  AP_WINGLEVELLER_HOLD,
  AP_GLIDESLOPE_HOLD,
  AP_AUTOTHROTTLEARMD_HOLD,
  AP_TOGA_HOLD,
  AP_TEST_BUTTON,
  // indexes into the 74HC165 control, therefore they must be in the same order as connected.  32 Max per PI74HC165Control class.
};

/*
  initialise parameters for switch inputs
*/
int pLoadPin74HC165 = 7; //PL on chip, pin 1
int clockEnablePin74HC165 = 9; //CE on chip, pin 15
int dataPin74HC165 = 6; // Q7 on chip, pin 9
int clockPin74HC165 = 8; //CP on chip, pin 2
#define NUMBER_OF_SHIFT_REGISTERS_74HC165 1 //although can handle up to 8, the return 'value' is limited to a long i.e. 32 (4 devices)

PI74HC165Control autoPilotSwitches = PI74HC165Control(pLoadPin74HC165, clockEnablePin74HC165, dataPin74HC165, clockPin74HC165, NUMBER_OF_SHIFT_REGISTERS_74HC165);

int timer=0;
void setup()
{
  Serial.begin(115200); //open serial port for communications
}

void loop()
{
Serial.println(timer);
    autoPilotSwitches.printState();

    autoPilotSwitches.update();//sets the previous switch states to the current ones.

  delay(POLL_DELAY_MSEC);
Serial.println(autoPilotSwitches.readState());
timer++;
}
