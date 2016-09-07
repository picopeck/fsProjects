/*
Tester for ShiftInControl
07/06/16 - reduced for ShiftIncontrol only

*/

#include "ShiftInControl.h"


const int POLL_DELAY_MSEC = 1;

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
#define NUMBER_OF_SHIFT_REGISTERS_74HC165 4 //although can handle up to 8, the return 'value' is limited to a long i.e. 32 (4 devices)

PI74HC165Control autoPilotSwitches = PI74HC165Control(pLoadPin74HC165, clockEnablePin74HC165, dataPin74HC165, clockPin74HC165, NUMBER_OF_SHIFT_REGISTERS_74HC165);

int iCodeIn, iStatus;
void AUTOPILOT_WRITE(eAPCONTROL);

void setup()
{
  Serial.begin(115200); //open serial port for communications
}

void loop()
{
  if (autoPilotSwitches.readState() != autoPilotSwitches.previousState()) //value is different
  {
    autoPilotSwitches.printState();

    processSwitchPositions(); //processes the current status of the switches

    autoPilotSwitches.update();//sets the previous switch states to the current ones.
  }

  delay(POLL_DELAY_MSEC);
}

void processSwitchPositions()  //will become the processor for switch statuses
{
  
  /* MAIN AUTOPILOT */
  if (autoPilotSwitches.isLOW2HIGH(AP_AUTOPILOT_HOLD))
    AUTOPILOT_WRITE(AP_AUTOPILOT_HOLD); //calls procedure to toggle the current state.
  /* END MAIN AUTOPILOT */

  /* ALTITUDE CONTROL */
  if (autoPilotSwitches.isClockwise(AP_ALTITUDE_UP, AP_ALTITUDE_DOWN))
  {
    AUTOPILOT_WRITE(AP_ALTITUDE_UP);
  }
  else if (autoPilotSwitches.isCounterClockwise(AP_ALTITUDE_UP, AP_ALTITUDE_DOWN))
  {
    AUTOPILOT_WRITE(AP_ALTITUDE_DOWN);
  }
  else { } //do nothing if the position hasn't changed

  if (autoPilotSwitches.isLOW2HIGH(AP_ALTITUDE_HOLD)) // value has transitioned from 0->1
    AUTOPILOT_WRITE(AP_ALTITUDE_HOLD);
  /* END ALTITUDE CONTROL */

  /* AIRSPEED CONTROL */
  // IASMACH Switch is independent of FS.  It is used as a way to control what is being displayed on the AP box.  this may be different to whats displayed in FS on startup
  if (autoPilotSwitches.isHIGH(AP_IASMACH_SWITCH))
  {
    //assumes '1' is for MACH
    Serial.println("AP_IASMACH_SWITCH is HIGH");
  }
  else if (autoPilotSwitches.isLOW(AP_IASMACH_SWITCH))
  {
    //assumes '0' is for IAS
    Serial.println("AP_IASMACH_SWITCH is LOW");
  }
  else {}

  if (autoPilotSwitches.isLOW2HIGH(AP_IASMACH_HOLD)) // value has transitioned from 0->1, can only 'HOLD' the active displayed speed type
  {
    AUTOPILOT_WRITE(AP_IASMACH_HOLD);
  }

  if (autoPilotSwitches.isClockwise(AP_IASMACH_UP, AP_IASMACH_DOWN))
  {
    AUTOPILOT_WRITE(AP_IASMACH_UP);
  }
  else if (autoPilotSwitches.isCounterClockwise(AP_IASMACH_UP, AP_IASMACH_DOWN))
  {
    AUTOPILOT_WRITE(AP_IASMACH_DOWN);
  }
  else { }
  /* END AIRSPEED CONTROL */

  /* FLIGHTDIRECTOR CONTROL */
  Serial.print("B30"); //write first part of message
  Serial.println(autoPilotSwitches.isHIGH(AP_FLIGHTDIRECTOR_SWITCH)); //write final part indicating whether ON or OFF
  /* END FLIGHTDIRECTOR CONTROL */

  /* AUTOTHROTTLE CONTROL */
  Serial.print("B34"); //write first part of message
  Serial.println(autoPilotSwitches.isHIGH(AP_AUTOTHROTTLE_SWITCH)); //write final part indicating whether ON or OFF
  /* END AUTOTHROTTLE CONTROL */

  /* HEADING CONTROL */
  // need to check status of the selector switch? is the encoder is dual purpose
  if (autoPilotSwitches.isClockwise(AP_HEADINGCOURSE_UP, AP_HEADINGCOURSE_DOWN))
  {
    AUTOPILOT_WRITE(AP_HEADINGCOURSE_UP);
  }
  else if (autoPilotSwitches.isCounterClockwise(AP_HEADINGCOURSE_UP, AP_HEADINGCOURSE_DOWN))
  {
    AUTOPILOT_WRITE(AP_HEADINGCOURSE_DOWN);
  }
  else { }

  if (autoPilotSwitches.isLOW2HIGH(AP_HEADING_HOLD)) // value has transitioned from 0->1
    AUTOPILOT_WRITE(AP_HEADING_HOLD);
  /* END HEADING CONTROL */

  /* VERTICALSPEED CONTROL */
  if (autoPilotSwitches.isClockwise(AP_VERTICALSPEED_UP, AP_VERTICALSPEED_DOWN))
  {
    AUTOPILOT_WRITE(AP_VERTICALSPEED_UP);
  }
  else if (autoPilotSwitches.isCounterClockwise(AP_VERTICALSPEED_UP, AP_VERTICALSPEED_DOWN))
  {
    AUTOPILOT_WRITE(AP_VERTICALSPEED_DOWN);
  }
  else { }

  if (autoPilotSwitches.isLOW2HIGH(AP_VERTICALSPEED_HOLD)) // value has transitioned from 0->1
    AUTOPILOT_WRITE(AP_VERTICALSPEED_HOLD);
  /* END VERTICALSPEED CONTROL */

  if (autoPilotSwitches.isLOW2HIGH(AP_APPROACH_HOLD)) // value has transitioned from 0->1
    AUTOPILOT_WRITE(AP_APPROACH_HOLD);

  if (autoPilotSwitches.isLOW2HIGH(AP_BACKCOURSE_HOLD)) // value has transitioned from 0->1
    AUTOPILOT_WRITE(AP_BACKCOURSE_HOLD);
}

void AUTOPILOT_WRITE(eAPCONTROL fFunction)
{
  /*code to write to link2fs when a value has changed via a Switch press or rotary change
    expecting it to be called with the appropriate function identifer
    define constants for all possible case.  think about enumerated type?
  */
  int tempBit;
  switch (fFunction)
  {
    case AP_ALTITUDE_UP:
      Serial.println("B11"); // increment request
      break;
    case AP_ALTITUDE_DOWN:
      Serial.println("B12"); // decrement request
      break;
    case AP_ALTITUDE_HOLD:
      Serial.println("B05"); //toggles the status
      break;
    case AP_IASMACH_HOLD:
      /* Assumption is that this will only be called if the switch has been detected as changing state from 0->1.  This could mean either OFF->ON or ON->OFF
        It is assumed that the current state will be inverted, as it hasn't been actioned to change yet.
        Need to check the status of the A/T switch, it can only be invoked if the switch is on
        Status(IASHOLD) | A/T | Action
        ================================
        OFF -> ON       |  0  | Nothing - invalid as IAS hold cannot be initiated if the A/T is not active
        OFF -> ON       |  1  | Toggle status - can be switched on
        ON -> OFF       |  0  | Nothing - should never be in this situation as Turning A/T OFF should disable MACH/IAS hold
        ON -> OFF       |  1  | Toggle status - can be switched off
        Therefore only need to perform an action if the A/T is ON
      */

      if (autoPilotSwitches.isHIGH(AP_AUTOTHROTTLE_SWITCH))
      {
        if (autoPilotSwitches.isLOW(AP_IASMACH_SWITCH))
        {
          Serial.println("B26"); //send IAS hold command.  This will automatically cause the MACH to be disabled and reflected back via link2fs
        }
        else
        {
          Serial.println("B20"); //send MACH hold command.  This will automatically cause the IAS to be disabled and reflected back via link2fs
        }
      }

      break;
    case AP_IASMACH_UP:
      if (autoPilotSwitches.isHIGH(AP_IASMACH_SWITCH))
      {
        Serial.println("B18"); // increment mach request
      }
      else
      {
        Serial.println("B15");//increment ias request
      }
      break;
    case AP_IASMACH_DOWN:
      if (autoPilotSwitches.isHIGH(AP_IASMACH_SWITCH))
      {
        Serial.println("B19"); // decrement mach request
      }
      else
      {
        Serial.println("B16");//decrement ias request
      }
      break;
    case AP_AUTOPILOT_HOLD:
      Serial.println("B01"); //toggle the status.
      break;
    /*case AP_FLIGHTDIRECTOR_SWITCH:
      WHAT FUNCTIONS CAUSE THE F/D TO BE ENABLED...
      This is called everytime a switch is detected as changing, to ensure FS relfects the current position.  If this disables any 'holds' then this will be reflected back to wordLEDStatuses
      break;
    */
    case AP_HEADINGCOURSE_UP:
      Serial.println("A57"); // increment request for heading.
      //  Can course be adjusted or is it determined by that set on NAV1?
      break;
    case AP_HEADINGCOURSE_DOWN:
      Serial.println("A58"); // decrement request for heading
      break;
    case AP_HEADING_HOLD:
      Serial.println("B04"); //toggles the status
      break;
    case AP_VERTICALSPEED_UP:
      Serial.println("B13"); // increment request
      break;
    case AP_VERTICALSPEED_DOWN:
      Serial.println("B14"); // decrement request
      break;
    //    IS THIS NEEDED? OR DOES ALT HOLD DO THE SAME THING?
    //    case AP_VERTICALSPEED_HOLD:
    //      Serial.println("B05"); //toggles the status
    //      break;
    case AP_APPROACH_HOLD:
      Serial.println("B08"); //toggles the status
      break;
    case AP_BACKCOURSE_HOLD:
      Serial.println("B09"); //toggles the status
      break;
    default:
      break;
  }
}
