/*
 test ShiftOutControl
 */
#include <LedControl.h>
#include "Controls.h"
#include "max7219Control.h"
#include "ShiftInControl.h"
#include "ShiftOutControl.h"

const int POLL_DELAY_MSEC = 1;

//pin allocations
//7 segment output displays
int max7219DataIn = 13;
int max7219CLK = 12;
int max7219LOAD = 11;
#define NUMBER_OF_MAX7219_DEVICES 1

PImax7219Control max7219Control = PImax7219Control(max7219DataIn, max7219CLK, max7219LOAD, NUMBER_OF_MAX7219_DEVICES);

// 7 segment device IDs
const int DEVICEID_VERTICALSPEED = 2;
const int DEVICEID_ALTITUDE = 1;
const int DEVICEID_HEADING = 0;
const int DEVICEID_COURSE = 0;
const int DEVICEID_IAS = 0;
const int DEVICEID_MACH = 0;
const int DEVICEID_RADIO_COMM1 = 99;
const int DEVICEID_RADIO_COMM1_STBY = 98;
//etc...


//LED status outputs
int latchPin74HC565N = 5; //chip pin 12
int clockPin74HC565N = 3; //chip pin 11
int dataPin74HC565N = 4; //chip pin 14
#define NUMBER_OF_SHIFT_REGISTERS_74HC565N 2
#define DATA_WIDTH_74HC565 NUMBER_OF_SHIFT_REGISTERS_74HC565N * 8

PI74HC565Control device74HC565 = PI74HC565Control(latchPin74HC565N, clockPin74HC565N, dataPin74HC565N, NUMBER_OF_SHIFT_REGISTERS_74HC565N);
// possibility to contain the 74HC565 within the Controls library.  Afterall they are all contained together.
//unsigned long current74HC165State; //global to hold the current state of the switch inputs.

/*
  initialise parameters for switch inputs
*/
int pLoadPin74HC165 = 7; //PL on chip, pin 1
int clockEnablePin74HC165 = 9; //CE on chip, pin 15
int dataPin74HC165 = 6; // Q7 on chip, pin 9
int clockPin74HC165 = 8; //CP on chip, pin 2
#define NUMBER_OF_SHIFT_REGISTERS_74HC165 1

PI74HC165Control device74HC165 = PI74HC165Control(pLoadPin74HC165, clockEnablePin74HC165, dataPin74HC165, clockPin74HC165, NUMBER_OF_SHIFT_REGISTERS_74HC165);

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

enum eAPFUNCTION
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
  AP_TEST_BUTTON
  //etc
  // indexes into the 74HC165 control, therefore they must be in the same order as connected
};

PIControls autoPilotSwitches = PIControls(21);

unsigned long current74HC165State;

enum eAP_HOLD_STATUSES
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
  // indexs into the 74HC565 array
};


//setup variables to hold local state of displays
long apAltitude;
int apHeading;
int apCourse ;
int apIAS;
int apMach ; //need to careful to multiply by the power when handling.
int apVerticalSpeed ;

int iCodeIn, iStatus;
void AUTOPILOT_READ(void);
void AUTOPILOT_WRITE(eAPFUNCTION);
void processSwitchPositions(void);

void setup()
{
  Serial.begin(115200); //open serial port for communications
  max7219Control.testDisplay();
}

void processSwitchPositions()  //will become the processor for switch statuses
{
  int tempBit, tempBit1;

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
    //displayNumber(DEVICE_MACH, apMach, 2, false); // display the current MACH value
    max7219Control.displayNumber(DEVICEID_MACH, apMach, 6, 3, 2); // display the current MACH value
  }
  else if (autoPilotSwitches.isLOW(AP_IASMACH_SWITCH))
  {
    //assumes '0' is for IAS
    //displayNumber(DEVICE_IAS, apIAS, 0, false); // display the current IAS value
    max7219Control.displayNumber(DEVICEID_IAS, apIAS, 6, 3);
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

void loop()
{
  Serial.println("looping");

  current74HC165State = device74HC165.readState();
  if (current74HC165State != device74HC165.previousState()) //value is different
  {
    Serial.println("*Pin Value change detected\r\n");

    autoPilotSwitches.setState(current74HC165State);//sets the controls to the current state of the 74HC165 device

    processSwitchPositions(); //processes the current status of the switches

    autoPilotSwitches.update();//sets the previous switch states to the current ones.
  }

  device74HC565.update(); // sends the bytes to the 74HC565 driver

  if (Serial.available()) //check for data on the serial bus
  {
    iCodeIn = getChar();
    if (iCodeIn == '=') //sign for Autopilot functions
    {
      AUTOPILOT_READ();
    }
  }

  delay(POLL_DELAY_MSEC);
}

//void setControlsState(unsigned long fValue)
//{
//  //sets all the control states
//  altitudeEncoder.setState(fValue);
//  verticalSpeedEncoder.setState(fValue);
//  heading_courseEncoder.setState(fValue);
//  ias_machEncoder.setState(fValue);
//
//  autopilotHoldSwitch.setState(fValue);
//  altitudeHoldSwitch.setState(fValue);
//  headingHoldSwitch.setState(fValue);
//  courseHoldSwitch.setState(fValue);
//  iasHoldSwitch.setState(fValue);
//  machHoldSwitch.setState(fValue);
//  approachHoldSwitch.setState(fValue);
//  backCourseHoldSwitch.setState(fValue);
//  verticalSpeedHoldSwitch.setState(fValue);
//
//  //etc
//}
//
//void setLastControlsState()
//{
//  //sets the previous state of each control to the current one
//  altitudeEncoder.update();
//  verticalSpeedEncoder.update();
//  heading_courseEncoder.update();
//  ias_machEncoder.update();
//
//  autopilotHoldSwitch.update();
//  altitudeHoldSwitch.update();
//  headingHoldSwitch.update();
//  courseHoldSwitch.update();
//  iasHoldSwitch.update();
//  machHoldSwitch.update();
//  approachHoldSwitch.update();
//  backCourseHoldSwitch.update();
//  verticalSpeedHoldSwitch.update();
//  device74HC165.update();
//
//  //etc
//}


/*//need code to handle the interrupt caused by a rotary/selection
  void interruptController()
  {
  //an interrupt has been detected
  //this should be stored when the interrupt is detected.
  pinValues74HC165 = readSwitchRegisters();

  if (pinValues74HC165 != oldPinValues74HC165) //value is different
  {
    //Serial.print("*Pin Value change detected\r\n");
    processSwitchPositions();
    oldPinValues74HC165 = pinValues74HC165;
  }
  //Serial.println("interrupt controller...");
  }
*/

char getChar()
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
  */
  int cTempChar = 0;
  String lclStrTemp = "";
  float fTemp;
  for (int iLoopVar = 0; iLoopVar < numChars; iLoopVar++)
  {
    cTempChar = getChar();
    if (((cTempChar > 0) && (cTempChar < 10)) || (cTempChar == '.') || (cTempChar == '-'))
    {
      lclStrTemp += cTempChar;  //do i need to exit the loop if 'getChar' is an '=' or other qualifier i.e. not a number or a valid character such as '-'
    }
    else break;
  }
  fTemp = lclStrTemp.toFloat();
  fTemp = fTemp * pow(10, iPower);
  return ((int)fTemp);
}

int getInt(int fNumChars, int fPower = 0)
{
  //function to iterate the fNumChars number of characters and return the integer value i.e. for headings, altitude, vert speed

  String sTemp = "";
  //need to get next fNumChars characters
  for (int iLoopVar = 0; iLoopVar < fNumChars; iLoopVar++)
  {
    sTemp += getChar();
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

long getLng(int fNumChars)
{
  //function to iterate the fNumChars number of characters and return the integer value i.e. for headings, altitude, vert speed

  String sTemp = "";
  //need to get next fNumChars characters
  for (int iLoopVar = 0; iLoopVar < fNumChars; iLoopVar++)
  {
    sTemp += getChar();
  }
  //    float temp;
  //    temp = (sTemp.toFloat());

  return (sTemp.toFloat());
}

void AUTOPILOT_WRITE(eAPFUNCTION fFunction)
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
        if (autoPilotSwitches.isHIGH(AP_IASMACH_SWITCH))
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
  iCodeIn = getChar(); //get next character from received string
  switch (iCodeIn)
  {
    case 'a': //autopilot active
      { //Serial.print("Autopilot Active Message ");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, AUTOPILOT_STATUS, iStatus - 48);
        device74HC565.setLed(AUTOPILOT_STATUS, iStatus - 48);
        break;
      }
    case 'b': //autopilot altitude setting
      { apAltitude = getLng(5);

        max7219Control.displayNumber(DEVICEID_ALTITUDE, apAltitude, 4, 5);
        break;
      }
    case 'c': //autopilot Vertical Speed Set
      { //Serial.println("Vertical Speed Set");
        apVerticalSpeed = getInt(5, 0);
        //Serial.println(apVerticalSpeed);
        int iNegative = 0;
        if (apVerticalSpeed < 0)
        {
          iNegative = -1;
          apVerticalSpeed *= iNegative;
        }
        else {
          iNegative = 0;
        }
        max7219Control.displayNumber(DEVICEID_VERTICALSPEED, apVerticalSpeed, 5, 4, 0, iNegative);
        break;
      }
    case 'd': //autopilot Heading Set
      { //Serial.print("Heading Set...");
        apHeading = getInt(3, 0);
        if (autoPilotSwitches.isHIGH(AP_HEADINGCOURSE_SWITCH));
        {
          max7219Control.displayNumber(DEVICEID_HEADING, apHeading, 6, 3);
          break;
        }
      }
    case 'e': //autopilot Course Set
      { //Serial.println("Course Set");
        apCourse = getInt(3, 0);
        //Serial.println(apCourse);
        if (autoPilotSwitches.isLOW(AP_HEADINGCOURSE_SWITCH));
        {
          max7219Control.displayNumber(DEVICEID_COURSE, apCourse, 6, 3);
          break;
        }
      }
    case 'f': //IAS setting
      { apIAS = getInt(3, 0);

        if (autoPilotSwitches.isHIGH(AP_IASMACH_SWITCH)) //ias_machSwitch.isLOW(0))
        {
          //if speed greater than 899 then put a decimal and the last two digits
          if (apIAS > 899)
          {
            max7219Control.displayNumber(DEVICEID_IAS, (apIAS - 900), 6, 3, 2, 1, false);
          }
          else
          {
            max7219Control.displayNumber(DEVICEID_IAS, apIAS, 6, 3);
          }
        }

        break;
      }
    case 'g': //MACH setting
      { apMach = getInt(4, 2);
        if (autoPilotSwitches.isLOW(AP_IASMACH_SWITCH))
        {
          max7219Control.displayNumber(DEVICEID_MACH, apMach, 1, 3, 2);
        }
        break;
      }
    case 'h': //autopilot MACH Hold
      { //Serial.print("MACH Hold...");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, MACHHOLD_STATUS, iStatus - 48);
        device74HC565.setLed(MACHHOLD_STATUS, iStatus - 48);
        break;
      }

    case 'j': //autopilot Heading Hold
      { //Serial.println("Heading Hold");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, HEADINGHOLD_STATUS, iStatus - 48);
        device74HC565.setLed(HEADINGHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'k': //autopilot Altitude Hold
      { //Serial.println("Altitude Hold");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, ALTITUDEHOLD_STATUS, iStatus - 48);
        device74HC565.setLed(ALTITUDEHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'l': //autopilot GPS drives NAV1
      { //Serial.println("GPS Drives NAV1");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, GPSDRIVESNAV1HOLD_STATUS, iStatus - 48);
        device74HC565.setLed(GPSDRIVESNAV1HOLD_STATUS, iStatus - 48);
        break;
      }
    case 'm': //autopilot Approach Hold
      { //Serial.println("Approach Hold");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, APPROACHHOLD_STATUS , iStatus - 48);
        device74HC565.setLed(APPROACHHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'n': //autopilot Backcourse
      { //Serial.println("Backcourse");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, BACKCOURSEHOLD_STATUS, iStatus - 48);
        device74HC565.setLed(BACKCOURSEHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'o': //autopilot NAV1 Lock
      { //Serial.println("NAV1 Lock");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, NAV1HOLD_STATUS, iStatus - 48);
        device74HC565.setLed(NAV1HOLD_STATUS, iStatus - 48);
        break;
      }
    case 'p': //autopilot Wing Leveller
      { //Serial.println("Wing Leveller");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, WINGLEVELLERHOLD_STATUS, iStatus - 48);
        device74HC565.setLed(WINGLEVELLERHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'q': //autopilot Flight Director
      { //Serial.println("Flight Director");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, FLIGHTDIRECTORHOLD_STATUS, iStatus - 48);
        device74HC565.setLed(FLIGHTDIRECTORHOLD_STATUS, iStatus - 48);
        break;
      }
    case 'r': //autopilot Glideslope Hold
      { //Serial.println("Glideslope Hold");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, GLIDESLOPEHOLD_STATUS, iStatus - 48);
        device74HC565.setLed(GLIDESLOPEHOLD_STATUS, iStatus - 48);
        break;
      }
    case 's': //autopilot IAS Hold
      { //Serial.print("IAS Hold...");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, IASHOLD_STATUS, iStatus - 48);
        device74HC565.setLed(IASHOLD_STATUS, iStatus - 48);
        //displayNumber(DEVICE_IAS, apIAS);

        break;
      }
    case 't': //autopilot Autothrottle Armed
      { //Serial.println("Autothrottle Armed");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, AUTOTHROTTLEARMED_STATUS, iStatus - 48);
        device74HC565.setLed(AUTOTHROTTLEARMED_STATUS, iStatus - 48);
        break;
      }
    case 'u': //autopilot Autothrottle Active
      { //Serial.println("Autothrottle");
        iNextChar = getChar();
        iStatus = (int)iNextChar;
        //bitWrite(wordLEDStatuses, AUTOTHROTTLEHOLD_STATUS, iStatus - 48);
        device74HC565.setLed(AUTOTHROTTLEHOLD_STATUS, iStatus - 48);
        break;
      }
  }
}

