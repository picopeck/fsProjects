/*
  PeckIndustrie RadioStack for fs.
  link2fs supplied by your buddy.
*/

#include "PIFunctions.h"
#include "ShiftInControl.h"
#include "max7219Control.h"


// 7 segment device IDS
const int DEVICEID_ACTIVEFREQUENCY = 0;
const int DEVICEID_STANDBYFREQUENCY = 1;
const int DEVICEID_TRANSPONDER = 2;

const int POLL_DELAY_MSEC = 1;

const bool DEBUG_ON = true;

typedef struct
{
  long activeFrequency;
  long stbyFrequency;
  long increment;
  long maxFrequency;
  long minFrequency;
} radioType;

//enum eAnalogInputs {A0, A1, A2, A3, A4, A5, A6, A7};

// all the radio controls
enum eRadios {ERROR, COMM1, COMM2, NAV1, NAV2, ADF, XPDR};

enum eRSCONTROL { RS_FREQUENCY_UP, RS_FREQUENCY_DOWN, RS_TEST_BUTTON, RS_TRANSFER_BUTTON, RS_MAJOR_FREQUENCY_SWITCH};

// ### Pin allocations ###
int max7219DataIn = 13;
int max7219CLK = 12;
int max7219LOAD = 11;
#define NUMBER_OF_max7219_DEVICES 3

PImax7219Control max7219Control = PImax7219Control(max7219DataIn, max7219CLK, max7219LOAD, NUMBER_OF_max7219_DEVICES);
PIFunctions commonFunction;

/*
  Initialise parameters for the HC165 devices
*/
int pLoadPin74HC165 = 7; //PL on chip, pin 1
int clockEnablePin74HC165 = 9; //CE on chip, pin 15
int dataPin74HC165 = 6; // Q7 on chip, pin 9
int clockPin74HC165 = 8; //CP on chip, pin 2.
#define SHIFT_IN_REGISTER_COUNT 1 //number of devices to initialise (up to 4)

PI74HC165Control radioStackSwitches = PI74HC165Control(pLoadPin74HC165, clockEnablePin74HC165, dataPin74HC165, clockPin74HC165, SHIFT_IN_REGISTER_COUNT);

bool IBITRunning = false;
bool dataInitialised = false; //used to store the data recieved from fs, so as to only do it once at startup.
int activeRadio;
radioType allRadios[7]; //one for each enumerated switch position
int activeXPDRDigit = 0;
bool bWasXPDR = false;

void RADIOSTACK_READ(void);
void initDefaults(void);
void processswitchPositions(void);
int analogSwitchPosition(int fIndex, int numContacts);
void RS_IBIT(void);
void initDefaults(void);
void processSwitchPosition(void);
void swapFrequencies(void);
void drawDisplay(void);
long coerceFrequency(bool bIncrement);
long readFrequency(eRadios fRadio);
int initTimer = 0;

void setup()
{
  Serial.begin(115200);
  initDefaults();
  max7219Control.initDevices();
  //device74HC595.IBIT(pow(2,32)-1);  // used for any indicator lights.  hoping to have at least 1 for the 'test' button.  may be on a dedicated pin if not.
}

void loop()
{
  if (initTimer < 500)
  {
    initTimer += 1;
    if (!dataInitialised) {
      RADIOSTACK_READ();
    }
    else {
      initTimer = 999;
    }
  }

  if (!IBITRunning)
  {
    // during 'loop' cycle, need to check which radio is selected from the main switch
    // if more than 1 index is used, then this could be enumerated too, but as only 1 for now, hard indexed
    activeRadio = analogSwitchPosition(A0, 6); // returns the switch position that is selected for the indexed switch, where 0 is an error. i.e. the active radio
    if (activeRadio == XPDR) bWasXPDR = true;

    if (radioStackSwitches.readState() != radioStackSwitches.previousState()) //switches have changed
    {
      Serial.println("Switch Change Detected...");
      processSwitchPositions();
      radioStackSwitches.update();
    }
    drawDisplay();
  }

  delay(POLL_DELAY_MSEC);
}


/*
  returns the switch position that the given analog input has detected
  int switchPosition : the index 0 to 7 of the available analog inputs on the Arduino UNO that has the required switch mounted.
  int numContacts : the number of contacts that the switch has.
  Usage : call the function and assign a variable to hold the returned switch position from 0 to 'numContacts'.  0 indicates an error.  switch positions are number clockwise, relative to the first position.  Assumption is that the switch can't go 360degs.
*/
int analogSwitchPosition(int fIndex, int numContacts)
{
  if (DEBUG_ON) Serial.println("analogSwitchPositions...");
  int switchPosition = ERROR; //ERROR condition, it means it hasn't found a valid position.
  int scale = (1024 / numContacts);
  int pinVoltage; //variable to store the sampled digitised voltage 0-1023 full scale.
  pinVoltage = analogRead(fIndex);
  for (int i = 0 ; i < numContacts; i++)
  {
    //if ((pinVoltage >= scale * i) && (pinVoltage <= (((scale * i) + scale) – 1)))
    if ((pinVoltage>=scale*i) && (pinVoltage<=((scale*i)+scale)))
    {
      switchPosition = i + 1;
    }
    //need to understand the hysteresis/noise  on the sampler, hopefully the samples will be in the middle of the range.
  }
  return (switchPosition);
  if (DEBUG_ON) Serial.println("Complete.");
}

void RS_IBIT()
{
  if (DEBUG_ON) Serial.println("IBIT Running...");
  IBITRunning = true;
  max7219Control.IBIT(); //turns on all the characters of each 7-segment
  //device74HC595.test();
  IBITRunning = false;
  if (DEBUG_ON) Serial.println("Complete.");
}

void initDefaults()
{
  if (DEBUG_ON) Serial.println("Init Defaults...");
  //remember no decimals, i.e. value in kHz
  allRadios[COMM1].activeFrequency = 123750;
  allRadios[COMM1].stbyFrequency = 123750;
  allRadios[COMM1].increment = 250;
  allRadios[COMM1].maxFrequency = 128;
  allRadios[COMM1].minFrequency = 118;
  allRadios[COMM2].activeFrequency = 123750;
  allRadios[COMM2].stbyFrequency = 123750;
  allRadios[COMM2].increment = 250;
  allRadios[COMM2].maxFrequency = 128;
  allRadios[COMM2].minFrequency = 118;
  allRadios[NAV1].activeFrequency = 112750;
  allRadios[NAV1].stbyFrequency = 115250;
  allRadios[NAV1].increment = 250;
  allRadios[NAV1].maxFrequency = 117;
  allRadios[NAV1].minFrequency = 108;
  allRadios[NAV2].activeFrequency = 110250;
  allRadios[NAV2].stbyFrequency = 111500;
  allRadios[NAV2].increment = 250;
  allRadios[NAV2].maxFrequency = 117;
  allRadios[NAV2].minFrequency = 108;
  allRadios[XPDR].activeFrequency = 7777; //not used
  allRadios[XPDR].stbyFrequency = 7777;
  allRadios[XPDR].increment = 1; //not used.  Always assumes +/-1 for each digit
  allRadios[XPDR].maxFrequency = 9999; //not used.  Always assumes +/-1 for each digit
  allRadios[XPDR].minFrequency = 0000; //not used.  Always assumes +/-1 for each digit
  allRadios[ADF].activeFrequency = 365;
  allRadios[ADF].stbyFrequency = 365;
  allRadios[ADF].increment = 1;
  allRadios[ADF].maxFrequency = 999;
  allRadios[ADF].minFrequency = 0;

  // read data from fs string and overwrite the current and stby frequencies accordingly.
  RADIOSTACK_READ();
  if (DEBUG_ON) Serial.println("Complete.");
}

void processSwitchPositions()
{
  if (DEBUG_ON) Serial.println("processSwitchPosition...");
  /*  IBIT  */
  if (radioStackSwitches.isHIGH(RS_TEST_BUTTON))
  {
    RS_IBIT();
  }

  /* Encoder Clockwise */
  if (radioStackSwitches.isClockwise(RS_FREQUENCY_UP, RS_FREQUENCY_DOWN)) //on encoder
  {
    // RS_MAJOR_FREQUENCY is a independent switch that determines whether to increment the fractional or integer part.
    allRadios[activeRadio].stbyFrequency = coerceFrequency(true);
  }

  /* Encoder CounterClockwise */
  if (radioStackSwitches.isCounterClockwise(RS_FREQUENCY_UP, RS_FREQUENCY_DOWN))
  {
    allRadios[activeRadio].stbyFrequency = coerceFrequency(false);
  }

  /* Transfer Button */
  if (radioStackSwitches.isLOW2HIGH(RS_TRANSFER_BUTTON))
  {
    if (activeRadio == XPDR)
    {
      if (activeXPDRDigit == 4) activeXPDRDigit = 1;
      else activeXPDRDigit += 1;
    }
    else
    {
      swapFrequencies();
    }
  }
  if (DEBUG_ON) Serial.println("Complete.");
}

void swapFrequencies()
{
  if (DEBUG_ON) Serial.println("swapFrequencies...");
  long tempFrequency = allRadios[activeRadio].activeFrequency;
  allRadios[activeRadio].activeFrequency = allRadios[activeRadio].stbyFrequency;
  allRadios[activeRadio].stbyFrequency = tempFrequency;
  //'output' the values to fs
  Serial.println("Outputting Frequency to fs");
  if (DEBUG_ON) Serial.println("Complete.");
}

void drawDisplay()
{
  if (DEBUG_ON) Serial.println("drawDisplay...");
  /* Selector switch has moved off XPDR */
  //always want to draw XPDR, unless it hasn't changed
  if (activeRadio == XPDR) //for when it's selected and the decimal denotes the digit which is changing
  {
    int tempXPDRFrequency;
    long tempXPDRFrequency2 = allRadios[XPDR].stbyFrequency;

    for (int i = 0; i < 4; i++)
    {
      tempXPDRFrequency = tempXPDRFrequency2 / pow(10, (4 - (i + 1)));
      tempXPDRFrequency2 -= tempXPDRFrequency * pow(10, (4 - (i + 1)));

      max7219Control.displayNumber(DEVICEID_TRANSPONDER, tempXPDRFrequency, i + 4, 1, activeXPDRDigit == i); //want to draw a decimal point on the activeXPDRDigit
      //note that it starts from the 4th digit as it uses the second half of a display.
    }
  }
  else if (bWasXPDR) // it won't be XPDR as the above would have taken place.
  { // the switch has moved from XPDR to something else, therefore need to send the XPDR frequency.
    max7219Control.displayNumber(DEVICEID_TRANSPONDER, allRadios[XPDR].stbyFrequency, 4, 4); //REQUIRES LEADING ZERO, another reason to perhaps have displayNumber as a string rather than numbers…
    Serial.println("send XPDR stbyFrequency");
    bWasXPDR = false;
  }
  else {}

  switch (activeRadio)
  {
    case ERROR:
      {
        Serial.println("### NO RADIO SELECTED ###");
        break;
      }
    case XPDR:
      {
        //do nothing additional to above conditioning
        break;
      }
    case ADF:
      {
        break;
      }
    default : //for all other radios, the frequencies of the active one are displayed
      {
        max7219Control.displayNumber(DEVICEID_ACTIVEFREQUENCY, allRadios[activeRadio].activeFrequency, 1, 6, 3 ); //use a decimal point in the 3rd position.
        max7219Control.displayNumber(DEVICEID_STANDBYFREQUENCY, allRadios[activeRadio].stbyFrequency, 3, 6, 3); //use 3rd position to shift by 2 to the right
        break;
      }
  }
  if (DEBUG_ON) Serial.println("Complete.");
}

/*
  ### coerceFrequency ### : returns a coerced frequency depending upon the radio selected.
  bIncrement : whether its incrementing (TRUE) or decrementing (FALSE)
  bMajor : whether it's the Major (integer) part (TRUE) or the fractional (FALSE) part
*/
long coerceFrequency(bool bIncrement)
{
  if (DEBUG_ON) Serial.println("coerceFrequency...");
  bool bMajor = radioStackSwitches.isHIGH(RS_MAJOR_FREQUENCY_SWITCH);
  long tempMajor = allRadios[activeRadio].stbyFrequency / 1000;
  long tempMinor = allRadios[activeRadio].stbyFrequency - tempMajor * 1000;

  switch (activeRadio)
  {
    case XPDR:
      {
        // could call a generic function which handles various numbers of field i.e. ADF will be the same code, but with 3 digits instead of 4
        // depends upon which digit is changing to how it wraps around…
        // work on each digit independently
        //split the frequency into 4 digits
        int tempXPDRFrequency[4];
        long tempXPDRFrequency2 = allRadios[XPDR].stbyFrequency;
        for (int i = 0; i < 4; i++)
        { // build the frequency one digit at a time
          tempXPDRFrequency[i] = tempXPDRFrequency2 / pow(10, (4 - (i + 1)));
          tempXPDRFrequency2 -= tempXPDRFrequency[i] * pow(10, (4 - (i + 1)));
        }
        //work on which one is changing.
        if (bIncrement)
        {
          if ((tempXPDRFrequency[activeXPDRDigit] + 1) > 9) tempXPDRFrequency[activeXPDRDigit] = 0;
          else tempXPDRFrequency[activeXPDRDigit] += 1;
        }
        else //decrementing
        {
          if ((tempXPDRFrequency[activeXPDRDigit] - 1) < 0) tempXPDRFrequency[activeXPDRDigit] = 9;
          else tempXPDRFrequency[activeXPDRDigit] -= 1;
        }

        //rebuild value
        tempMinor = 0;
        for (int i = 0; i < 4; i++)
        { // rebuild the frequency one digit at a time
          tempMinor += tempXPDRFrequency[i] * pow(10, (4 - (i + 1)));
        }
        tempMajor = 0;
        break;
      }
    case ADF:
      {
        break;
      }
Default:
      {
        if (bMajor)
        {
          //maxFrequency is the maximum integer part that can be given
          if (bIncrement)
          {
            if ((tempMajor + 1) > allRadios[activeRadio].maxFrequency) tempMajor = allRadios[activeRadio].minFrequency;
            else tempMajor += 1;
          }
          else
          {
            if ((tempMajor - 1) < allRadios[activeRadio].minFrequency) tempMajor = allRadios[activeRadio].maxFrequency;
            else tempMajor -= 1;
          }
        }
        else //it's working on the fractional part
          // values are fractional and hence can be compared to >=1000 or <0
        {
          if (bIncrement)
          {
            if ((tempMinor + allRadios[activeRadio].increment) >= 1000) tempMinor = 0;
            else tempMinor += allRadios[activeRadio].increment;
          }
          else
          {
            if ((tempMinor - allRadios[activeRadio].increment) < 0) tempMinor = (1000 - allRadios[activeRadio].increment);
            else tempMinor -= allRadios[activeRadio].increment;
          }
        }
        break;
      }
  }
  if (DEBUG_ON) Serial.println(tempMajor * 1000 + tempMinor);
  if (DEBUG_ON) Serial.println("Complete.");
  return (tempMajor * 1000 + tempMinor);
}

void RADIOSTACK_READ()
{
  if (DEBUG_ON) Serial.println("RADIOSTACK_READ...");

  Serial.print("initTimer : ");
  Serial.println(initTimer);

  int iCodeIn;
  if ((Serial.available()) && (!dataInitialised)) //checks for data on the bus and that the data hasn't hasn't already been read.
  {
    iCodeIn = commonFunction.getChar();
    if (iCodeIn == '=') //sign for radio data
    {
      iCodeIn = commonFunction.getChar(); //get the next character
      switch (iCodeIn)
      {
        case 'A': //com1 frequency
          {
            allRadios[COMM1].activeFrequency = readFrequency(COMM1);
            dataInitialised = true; //put this in all cases to ensure it gets set if at least one radio has been read in.
            break;
          }
        case 'B': //com1 stby frequency
          {
            allRadios[COMM1].stbyFrequency = readFrequency(COMM1);
            dataInitialised = true; //put this in all cases to ensure it gets set if at least one radio has been read in.
            break;
          }
        case 'C': //com2 frequency
          {
            allRadios[COMM2].activeFrequency = readFrequency(COMM2);
            dataInitialised = true; //put this in all cases to ensure it gets set if at least one radio has been read in.
            break;
          }
        case 'D': //com2 stby frequency
          {
            allRadios[COMM2].stbyFrequency = readFrequency(COMM2);
            dataInitialised = true; //put this in all cases to ensure it gets set if at least one radio has been read in.
            break;
          }
        case 'E': //Nav1 frequency
          {
            allRadios[NAV1].activeFrequency = readFrequency(NAV1);
            dataInitialised = true; //put this in all cases to ensure it gets set if at least one radio has been read in.
            break;
          }
        case 'F': //Nav1 stby frequency
          {
            allRadios[NAV1].stbyFrequency = readFrequency(NAV1);
            dataInitialised = true; //put this in all cases to ensure it gets set if at least one radio has been read in.
            break;
          }
        case 'G': //Nav2 frequency
          {
            allRadios[NAV2].activeFrequency = readFrequency(NAV2);
            dataInitialised = true; //put this in all cases to ensure it gets set if at least one radio has been read in.
            break;
          }
        case 'H': //Nav2 stby frequency
          {
            allRadios[NAV2].stbyFrequency = readFrequency(NAV2);
            dataInitialised = true; //put this in all cases to ensure it gets set if at least one radio has been read in.
            break;
          }
        case 'I': // ADF code
          {
            allRadios[ADF].activeFrequency = 0; //not used.  Always uses standby
            allRadios[ADF].stbyFrequency = readFrequency(ADF);
            dataInitialised = true; //put this in all cases to ensure it gets set if at least one radio has been read in.
            break;
          }
        case 'J': //transponder code
          {
            allRadios[XPDR].activeFrequency = 0; //not used.  Always uses standby
            allRadios[XPDR].stbyFrequency = readFrequency(XPDR);
            dataInitialised = true; //put this in all cases to ensure it gets set if at least one radio has been read in.
            break;
          }
      }
    }
  }
  if (DEBUG_ON) Serial.println("Complete.");
}

// reads the frequency from the string fs string
long readFrequency(eRadios fRadio)
{
  if (DEBUG_ON) Serial.println("readFrequency...");
  long tempFrequency;
  if (fRadio == ERROR)
  {
    Serial.println("### No Radio Selected [readFrequency] ###");
                 }
                   else if (fRadio==XPDR)
                 {
                   //length will be 4 characters
                   tempFrequency=commonFunction.getLong(4);
                 }
                   else if (fRadio==ADF)
                 {
                   //length will be 3 characters
                   tempFrequency=commonFunction.getLong(3);
                 }
                   else // all other radios will be 6 digits
                 {
                   //length will be 6 characters
                   tempFrequency=commonFunction.getInt(6,3); // need to add a decimal clause in the main PIFunctions to pass bacl a long.
                 }
                   if (DEBUG_ON) Serial.println(tempFrequency);
                   if (DEBUG_ON) Serial.println("Complete.");
                   return (tempFrequency);
                 }
