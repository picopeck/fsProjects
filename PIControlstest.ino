/*
  Tester program for PIControls
*/


#include <LedControl.h>
#include "Controls.h"
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
}

void loop()
{
    autoPilotSwitches.setState(current74HC165State);//sets the controls to the current state of the 74HC165 device

    autoPilotSwitches.update();//sets the previous switch states to the current ones.


  if (Serial.available()) //check for data on the serial bus
  {
    iCodeIn = getChar();
    if (iCodeIn == '=') //sign for Autopilot functions
    {
      AUTOPILOT_READ();
    }
  }

  delay(500);
}


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
    case 'b': //autopilot altitude setting
      { apAltitude = getLng(5);
        Serial.print("apAltitude :");
        Serial.println(apAltitude);

        break;
      }
    case 'e': //autopilot Course Set
      { //Serial.println("Course Set");
        apCourse = getInt(3, 0);
        //Serial.println(apCourse);
        if (autoPilotSwitches.isLOW(AP_HEADINGCOURSE_SWITCH));
        {
          Serial.print("apCourse :");
          Serial.println(apCourse);
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
            Serial.print("apIAS :");
            Serial.println(apIAS);
          }
        }

        break;
      }
    case 'g': //MACH setting
      { apMach = getInt(4, 2);
        if (autoPilotSwitches.isLOW(AP_IASMACH_SWITCH))
        {
          Serial.print("apMach :");
          Serial.println(apMach);
        }
        break;
      }
  }
}

