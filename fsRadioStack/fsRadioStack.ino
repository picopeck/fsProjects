/*
  PeckIndustries interface to flight sim for RadioStack module.
  Displays and LED controlling done by Arduino Uno
  Code taken from a multitude of sources, mainly Jim for link2fs program, and Arduino samples.
  Receives inputs via link2fs on the status of the autopilot system.
  v23/09/16 - omitted generic functions and added PIFunctions.h as a common repository.
*/

#include "PIFunctions.h"
#include "max7219Control.h"
#include "ShiftInControl.h"

const int POLL_DELAY_MSEC = 1;

// 7 segment device IDs
const int DEVICEID_COMM1 = 0;
const int DEVICEID_COMM1_STBY = 1;
const int DEVICEID_COMM2 = 2;
const int DEVICEID_COMM2_STBY = 3;
const int DEVICEID_NAV1 = 4;
const int DEVICEID_NAV1_STBY = 5;
const int DEVICEID_NAV2 = 6;
const int DEVICEID_NAV2_STBY = 7;
const int DEVICEID_ADF = 8;
const int DEVICEID_XPDR = 8;
// 1 too many displays.  is there enough spare pins to have another set?  Or use a common 'standby' radio

typedef struct
{
  long currentValue;
  long defaultValue = 0;
  long standbyValue;
  //plus any others
} controlType;

//setup variables to hold local state of variable things
//need to careful to multiply by the power when handling.
controlType rsCOMM1;
controlType rsCOMM2;
controlType rsNAV1;
controlType rsNAV2;
controlType rsADF; 
controlType rsXPDR;

//etc...
#define ON 1
#define OFF 0
#define INCREMENT 1
#define DECREMENT -1
#define HIGH2LOW -2
#define LOW2HIGH 2
#define NOCHANGE 0
#define NOCHANGE_HIGH 1
#define NOCHANGE_LOW -1
#define UNDETERMINED -99

enum eAPCONTROL
{
  AP_COMM1_UP,
  AP_COMM1_DOWN,
  AP_COMM1_STBY_SWITCH,
  AP_COMM2_UP,
  AP_COMM2_DOWN,
  AP_COMM2_STBY_SWITCH,
  AP_NAV1_UP,
  AP_NAV1_DOWN,
  AP_NAV1_STBY_SWITCH,
  AP_NAV2_UP,
  AP_NAV2_DOWN,
  AP_NAV2_STBY_SWITCH,
  AP_ADF_UP,
  AP_ADF_DOWN,
  AP_XPDR_UP,
  AP_XPDR_DOWN,
  // indexes into the 74HC165 control, therefore they must be in the same order as connected.  32 Max per PI74HC165Control class.
};

/*enum eAP_HOLD_STATUSES
{
  AUTOPILOT_STATUS,
  ALTITUDEHOLD_STATUS,
  HEADINGHOLD_STATUS,
  VERTSPEED_STATUS,
  IASHOLD_STATUS,
  MACHHOLD_STATUS,
  AUTOTHROTTLEARMED_STATUS,
  NAV1HOLD_STATUS,
  BACKCOURSEHOLD_STATUS,
  GPSDRIVESNAV1HOLD_STATUS,
  WINGLEVELLERHOLD_STATUS,
  FLIGHTDIRECTORHOLD_STATUS,
  GLIDESLOPEHOLD_STATUS,
  APPROACHHOLD_STATUS,
  AUTOTHROTTLEHOLD_STATUS,
  TOGAHOLD_STATUS
  // indexes into the 74HC565 array
};*/

// ### Pin allocations ###
//7 segment output displays
int max7219DataIn = 13;
int max7219CLK = 12;
int max7219LOAD = 11;
#define NUMBER_OF_MAX7219_DEVICES 8

PImax7219Control max7219Control = PImax7219Control(max7219DataIn, max7219CLK, max7219LOAD, NUMBER_OF_MAX7219_DEVICES);
PIFunctions commonFunction;
/*
  initialise parameters for switch inputs
*/
int pLoadPin74HC165 = 7; //PL on chip, pin 1
int clockEnablePin74HC165 = 9; //CE on chip, pin 15
int dataPin74HC165 = 6; // Q7 on chip, pin 9
int clockPin74HC165 = 8; //CP on chip, pin 2
#define NUMBER_OF_SHIFT_REGISTERS_74HC165 4 //although can handle up to 8, the return 'value' is limited to a long i.e. 32 (4 devices)

PI74HC165Control radioStackSwitches = PI74HC165Control(pLoadPin74HC165, clockEnablePin74HC165, dataPin74HC165, clockPin74HC165, NUMBER_OF_SHIFT_REGISTERS_74HC165);
//unsigned long stateRadioStackSwitches;

int iCodeIn, iStatus;
bool IBITRunning = false;
void RADIOSTACK_READ(void);
void RADIOSTACK_WRITE(eAPCONTROL);
void initDefaults(void);
void processSwitchPositions(void);

void setup()
{
  Serial.begin(115200); //open serial port for communications
  initDefaults();
  max7219Control.initDevices();
  //device74HC595.IBIT(pow(2,32)-1);
}

void loop()
{
  if (!IBITRunning)
  {
    if (Serial.available()) //check for data on the serial bus
    {
      iCodeIn = commonFunction.getChar();
      if (iCodeIn == '=') //sign for Autopilot functions
      {
        RADIOSTACK_READ();
      }
    }

    if (radioStackSwitches.readState() != radioStackSwitches.previousState()) //value is different
    {
      Serial.println("*Pin Value change detected\r\n");

      processSwitchPositions(); //processes the current status of the switches

     radioStackSwitches.update();//sets the previous switch states to the current ones.
    }

    //device74HC595.update(); // sends the bytes to the 74HC565 driver

    delay(POLL_DELAY_MSEC);
  }
}

// initialises the default state of things
void initDefaults()
{
  apAltitude.currentValue = apAltitude.defaultValue;
  apCourse.currentValue = apCourse.defaultValue;
  apIAS.currentValue = apIAS.defaultValue;
  apMach.currentValue = apMach.defaultValue; //need to careful to multiply by the power when handling.
  apVerticalSpeed.currentValue = apVerticalSpeed.defaultValue;;
  //could be expanded to include the momentaries too
}

/*
  Function to test the status of all the LEDs/displays of the autopilot.
  Testing will interrupt normal behaviour but should resume to previous settings upon completion
*/
void AP_IBIT()
{
  IBITRunning = true;
  max7219Control.IBIT(); //sets all the displays to '8' and decimal points
  //device74hc595.test(); turns on all the LEDs
  IBITRunning = false;
}

/*
  char commonFunction.getChar()
  {
  while (Serial.available() == 0);
  return ((char)Serial.read());
  }

  int getNumber(int numChars, int iPower)
  {
  /*
    will return the integer of the number of 'numChars' characters, and multiply by iPower if its a decimal
    INPUTS
          int numChars - the number of characters to read for the value
          int iPower - the power to 10 which to multiply the number by to enable only long numbers

  int cTempChar = 0;
  String lclStrTemp = "";
  float fTemp;
  for (int iLoopVar = 0; iLoopVar < numChars; iLoopVar++)
  {
    cTempChar = commonFunction.getChar();
    if (((cTempChar > 0) && (cTempChar < 10)) || (cTempChar == '.') || (cTempChar == '-'))
    {
      lclStrTemp += cTempChar;  //do i need to exit the loop if 'commonFunction.getChar' is an '=' or other qualifier i.e. not a number or a valid character such as '-'
    }
    else break;
  }
  fTemp = lclStrTemp.toFloat();
  fTemp = fTemp * pow(10, iPower);
  return ((int)fTemp);
  }

  int commonFunction.getInt(int fNumChars, int fPower = 0)
  {
  //function to iterate the fNumChars number of characters and return the integer value i.e. for headings, altitude, vert speed

  String sTemp = "";
  //need to get next fNumChars characters
  for (int iLoopVar = 0; iLoopVar < fNumChars; iLoopVar++)
  {
    sTemp += commonFunction.getChar();
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

  long commonFunction.getLong(int fNumChars)
  {
  //function to iterate the fNumChars number of characters and return the integer value i.e. for headings, altitude, vert speed

  String sTemp = "";
  //need to get next fNumChars characters
  for (int iLoopVar = 0; iLoopVar < fNumChars; iLoopVar++)
  {
    sTemp += commonFunction.getChar();
  }
  return (sTemp.toFloat());
  }

*/

void processSwitchPositions()  //will become the processor for switch statuses
{
  /* IBIT */
  if (autoPilotSwitches.isLOW2HIGH(AP_TEST_BUTTON)) // value has transitioned from 0->1
  {
    AP_IBIT();
  }

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
    max7219Control.displayNumber(DEVICEID_MACH, apMach.currentValue, 1, 3, 2); // display the current MACH value
  }
  else if (autoPilotSwitches.isLOW(AP_IASMACH_SWITCH))
  {
    //assumes '0' is for IAS
    max7219Control.displayNumber(DEVICEID_IAS, apIAS.currentValue, 1, 3);
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

void AUTOPILOT_READ()
{
  /*
    This code relies upon getting data from link2fs, which could create unnecessary traffic i.e. Switch change=>send to FS=>FS sends new number/freq/status=>update displays and indicators.
    This is great from a startup point of view, but could be better served being simply a transmitter of information, and a closed entity.
     i.e. on startup, send Arduino's derived status ergo it aligns with the current state.
          switch change=>send FS
                       =>modify arudino output on status, display number.
    Won't need to process changes from external FS inputs.  This function will be re-written to send the updated value to FS, based upon the switch pressed.
    Need to set default states for each of the displays, then send them to FS at startup
    link2fs does not provide an input to directly set the status of an AP function, it enables an increment/decrement, toggle capability.  Therefore this needs to remain.  Lag does not appear to be an issue.
  */

  long iNextChar;
  bool bTemp;
  iCodeIn = commonFunction.getChar(); //get next character from received string
  switch (iCodeIn)
  {
    case 'a': //autopilot active
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //        device74HC565.setLed(AUTOPILOT_STATUS, iStatus - 48);
        break;
      }
    case 'b': //autopilot altitude setting
      {
        apAltitude.currentValue = commonFunction.getLong(5);
        max7219Control.displayNumber(DEVICEID_ALTITUDE, apAltitude.currentValue, 4, 5);
        break;
      }
    case 'c': //autopilot Vertical Speed Set
      {
        apVerticalSpeed.currentValue = commonFunction.getInt(5, 0);
        int iNegative = 0;
        if (apVerticalSpeed.currentValue < 0)
        {
          iNegative = -1;
          apVerticalSpeed.currentValue *= iNegative;
        }
        else {
          iNegative = 0;
        }
        max7219Control.displayNumber(DEVICEID_VERTICALSPEED, apVerticalSpeed.currentValue, 5, 4, 0, iNegative);
        break;
      }
    case 'd': //autopilot Heading Set
      {
        apHeading.currentValue = commonFunction.getInt(3, 0);
        HDGorCOURSE();
        break;
      }
    case 'e': //autopilot Course Set
      {
        apCourse.currentValue = commonFunction.getInt(3, 0);
        HDGorCOURSE();
        break;
      }
    case 'f': //IAS setting
      {
        apIAS.currentValue = commonFunction.getInt(3, 0);
        IASorMACH();
        break;
      }
    case 'g': //MACH setting
      {
        apMach.currentValue = commonFunction.getInt(4, 2);
        IASorMACH();
        break;
      }
    case 'h': //autopilot MACH Hold
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //        device74HC565.setLed(MACHHOLD_STATUS, iStatus - 48);
        break;
      }

    case 'j': //autopilot Heading Hold
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //       device74HC565.setLed(HEADINGHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'k': //autopilot Altitude Hold
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //        device74HC565.setLed(ALTITUDEHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'l': //autopilot GPS drives NAV1
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //       device74HC565.setLed(GPSDRIVESNAV1HOLD_STATUS, iStatus - 48);
        break;
      }
    case 'm': //autopilot Approach Hold
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //      device74HC565.setLed(APPROACHHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'n': //autopilot Backcourse
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //        device74HC565.setLed(BACKCOURSEHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'o': //autopilot NAV1 Lock
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //        device74HC565.setLed(NAV1HOLD_STATUS, iStatus - 48);
        break;
      }
    case 'p': //autopilot Wing Leveller
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //        device74HC565.setLed(WINGLEVELLERHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'q': //autopilot Flight Director
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //        device74HC565.setLed(FLIGHTDIRECTORHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'r': //autopilot Glideslope Hold
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //        device74HC565.setLed(GLIDESLOPEHOLD_STATUS, iStatus - 48);
        break;
      }
    case 's': //autopilot IAS Hold
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //        device74HC565.setLed(IASHOLD_STATUS, iStatus - 48);
        break;
      }
    case 't': //autopilot Autothrottle Armed
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //        device74HC565.setLed(AUTOTHROTTLEARMED_STATUS, iStatus - 48);
        break;
      }
    case 'u': //autopilot Autothrottle Active
      {
        iNextChar = commonFunction.getChar();
        iStatus = (int)iNextChar;
        //        device74HC565.setLed(AUTOTHROTTLEHOLD_STATUS, iStatus - 48);
        break;
      }
  }
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

void HDGorCOURSE()
{
  if (autoPilotSwitches.isLOW(AP_HEADINGCOURSE_SWITCH))
  {
    if (apHeading.currentValue == 0) apHeading.currentValue = 360;
    max7219Control.displayNumber(DEVICEID_HEADING, apHeading.currentValue, 6, 3);
  }
  else if (autoPilotSwitches.isHIGH(AP_HEADINGCOURSE_SWITCH))
  {
    if (apCourse.currentValue == 0) apCourse.currentValue = 360;
    max7219Control.displayNumber(DEVICEID_COURSE, apCourse.currentValue, 6, 3);
  }
  else Serial.println("### Error in HEADINGCOURSE switch detection ###");
}

void IASorMACH()
{
  if (autoPilotSwitches.isLOW(AP_IASMACH_SWITCH)) //ias_machSwitch.isLOW(0))
  {
    //if speed greater than 899 then put a decimal and the last two digits
    if (apIAS.currentValue > 899)
    {
      max7219Control.displayNumber(DEVICEID_IAS, (apIAS.currentValue - 900), 1, 3, 2, 1, false);
    }
    else
    {
      max7219Control.displayNumber(DEVICEID_IAS, apIAS.currentValue, 1, 3);
    }
  }
  else if (autoPilotSwitches.isHIGH(AP_IASMACH_SWITCH))
  {
    //apMach = commonFunction.getInt(4, 2);
    max7219Control.displayNumber(DEVICEID_MACH, apMach.currentValue, 1, 3, 2);
  }
  else Serial.println("### Error in IASMACH switch detection ###");
}
