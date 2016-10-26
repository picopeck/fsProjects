/*
  PeckIndustries RadioStack for fs.
  link2fs supplied by your buddy.
26/10/16 modified switch enumerated types to align with radio selection switch
*/

#include "PIFunctions.h"
#include "ShiftInControl.h"
#include "max7219Control.h"

// 7 segment device IDS
const int DEVICEID_ACTIVEFREQUENCY = 0;
const int DEVICEID_STANDBYFREQUENCY = 1;
const int DEVICEID_TRANSPONDER = 2;

const int POLL_DELAY_MSEC = 100;

typedef struct
{
  long activeFrequency;
  long stbyFrequency;
  long increment;
  long maxFrequency;
  long minFrequency;
} radioType;

// all the radio controls
enum eRadios {NONE,COMM1, COMM2, NAV1, NAV2, ADF, XPDR};
char* radioTags[] = {"NONE", "COMM1", "COMM2", "NAV1", "NAV2", "ADF", "XPDR"};

enum eRSCONTROL {NAV2_SELECTED,a,NAV1_SELECTED,b,RS_FREQUENCY_DOWN,RS_FREQUENCY_UP,RS_TEST_BUTTON,RS_TRANSFER_BUTTON,COMM2_SELECTED,RS_MAJOR_FREQUENCY_SWITCH,d,e,f,COMM1_SELECTED,XPDR_SELECTED,ADF_SELECTED};
int radioSelectorSwitch[] = {COMM1_SELECTED, COMM2_SELECTED, NAV1_SELECTED, NAV2_SELECTED, ADF_SELECTED, XPDR_SELECTED};
#define RADIOSELECTORNUMCONTACTS 6

// ### Pin allocations ###
int max7219DataIn = 13; //DIN
int max7219CLK = 12; //CLK
int max7219LOAD = 11; //CS
#define NUMBER_OF_max7219_DEVICES 3

PImax7219Control max7219Control = PImax7219Control(max7219DataIn, max7219CLK, max7219LOAD, NUMBER_OF_max7219_DEVICES);
PIFunctions commonFunction;

/*
  Initialise parameters for the HC165 devices
*/
int pLoadPin74HC165 = 7; //PL on chip, pin 1, SH/LO
int clockEnablePin74HC165 = 9; //CE on chip, pin 15, CE
int dataPin74HC165 = 6; // Q7 on chip, pin 9, SER_OUT
int clockPin74HC165 = 8; //CP on chip, pin 2, CLK
#define SHIFT_IN_REGISTER_COUNT 2 //number of devices to initialise (up to 4)

PI74HC165Control radioStackSwitches = PI74HC165Control(pLoadPin74HC165, clockEnablePin74HC165, dataPin74HC165, clockPin74HC165, SHIFT_IN_REGISTER_COUNT);

bool IBITRunning = false;
bool dataInitialised = false; //used to store the data recieved from fs, so as to only do it once at startup.
int activeRadio;
int previousRadio = 0;
radioType allRadios[RADIOSELECTORNUMCONTACTS]; //one for each enumerated switch position
int activeXPDRDigit = 1;
int activeADFDigit = 1;
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
long readFrequency(eRSCONTROL fRadio);
int initTimer = 0;


void setup()
{
  Serial.begin(115200);
  max7219Control.initDevices();
  delay(1000);
  initDefaults();
  //device74HC595.IBIT(pow(2,32)-1);  // used for any indicator lights.  hoping to have at least 1 for the 'test' button.  may be on a dedicated pin if not.
}

void loop()
{
  //  Serial.print("initTimer :");
  //  Serial.println(initTimer);
  //  if (initTimer < 50)
  //  {
  //    initTimer += 1;
  //    if (!dataInitialised) {
  //      RADIOSTACK_READ();
  //    }
  //    else {
  //      initTimer = 999;
  //    }
  //  }
  //
  if (!IBITRunning)
  {
    if (radioStackSwitches.readState() != radioStackSwitches.previousState()) //switches have changed
    {
      processSwitchPositions();
      drawDisplay();
      radioStackSwitches.update();
    }
  }

  delay(POLL_DELAY_MSEC);
}

void RS_IBIT()
{
  IBITRunning = true;
  max7219Control.IBIT(); //turns on all the characters of each 7-segment
  //device74HC595.test();
  IBITRunning = false;
}

void initDefaults()
{
  //remember no decimals, i.e. value in kHz
  allRadios[COMM1].activeFrequency = 123750;
  allRadios[COMM1].stbyFrequency = 124750;
  allRadios[COMM1].increment = 250;
  allRadios[COMM1].maxFrequency = 128;
  allRadios[COMM1].minFrequency = 118;
  allRadios[COMM2].activeFrequency = 125750;
  allRadios[COMM2].stbyFrequency = 126750;
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
  allRadios[XPDR].stbyFrequency = 5678;
  allRadios[XPDR].increment = 1; //not used.  Always assumes +/-1 for each digit
  allRadios[XPDR].maxFrequency = 9999; //not used.  Always assumes +/-1 for each digit
  allRadios[XPDR].minFrequency = 0000; //not used.  Always assumes +/-1 for each digit
  allRadios[ADF].activeFrequency = 365;
  allRadios[ADF].stbyFrequency = 365;
  allRadios[ADF].increment = 1;
  allRadios[ADF].maxFrequency = 999;
  allRadios[ADF].minFrequency = 0;

  // read data from fs string and overwrite the current and stby frequencies accordingly.
  //  RADIOSTACK_READ();
  // run through each of the switch positions to load them up.
  for (int i = 0; i < RADIOSELECTORNUMCONTACTS; i++)
  {
    activeRadio = radioSelectorSwitch[i+1];
    drawDisplay();
  }
}

void processSwitchPositions()
{
  Serial.println("Entry...[processSwitchPosition]");

  /* IBIT Processes */
  if (radioStackSwitches.isHIGH(RS_TEST_BUTTON))
  {
    RS_IBIT();
  }


//returns the currently selected radio as a pin index from the given array
activeRadio = radioStackSwitches.digitalSwitchPosition(radioSelectorSwitch, RADIOSELECTORNUMCONTACTS);
//map the pin index to the actual radio.
switch (activeRadio)
{
case COMM1_SELECTED:
  {
    activeRadio=COMM1;
    break;
  }
case COMM2_SELECTED:
  {
    activeRadio=COMM2;
    break;
  }
case NAV1_SELECTED:
  {
    activeRadio=NAV1;
    break;
  }
case NAV2_SELECTED:
  {
    activeRadio=NAV2;
    break;
  }
case ADF_SELECTED:
  {
    activeRadio=ADF;
    break;
  }
case XPDR_SELECTED:
  {
    activeRadio=XPDR;
    break;
  }
default:
  {
    Serial.println("### ILLEGAL RADIO - [processSwitchPositions] ####");
    activeRadio=COMM1;
    break;
  }
}


    Serial.print("Active Radio :");
    Serial.println(radioTags[activeRadio]);

  if (activeRadio != previousRadio)
  {
    drawDisplay();
  }
  previousRadio = activeRadio;
  
if (activeRadio == XPDR) bWasXPDR = true;

  /* Encoder Clockwise */
  //if (radioStackSwitches.isClockwise(RS_FREQUENCY_UP, RS_FREQUENCY_DOWN)) //on encoder
  //if (radioStackSwitches.isLOW2HIGH(RS_FREQUENCY_UP))
  if (radioStackSwitches.isHIGH(RS_FREQUENCY_UP))  // want to keep incrementing if it's HIGH
  {
    Serial.println("incrementing");
    // RS_MAJOR_FREQUENCY is a independent switch that determines whether to increment the fractional or integer part.
    allRadios[activeRadio].stbyFrequency = coerceFrequency(true);
  }

  /* Encoder CounterClockwise */
  //if (radioStackSwitches.isCounterClockwise(RS_FREQUENCY_UP, RS_FREQUENCY_DOWN))
  //if (radioStackSwitches.isLOW2HIGH(RS_FREQUENCY_DOWN))
  if (radioStackSwitches.isHIGH(RS_FREQUENCY_DOWN))  // want to keep decrementing if it's HIGH
  {
    Serial.println("decrementing");
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
    else if (activeRadio == ADF)
    {
      if (activeADFDigit == 3) activeADFDigit = 1;
      else activeADFDigit += 1;
    }
    else
    {
      swapFrequencies();
    }
  }
  Serial.println("Exit - [processSwitchPosition].");
}

void swapFrequencies()
{
  long tempFrequency = allRadios[activeRadio].activeFrequency;
  allRadios[activeRadio].activeFrequency = allRadios[activeRadio].stbyFrequency;
  allRadios[activeRadio].stbyFrequency = tempFrequency;
  //'output' the values to fs
  Serial.println("Outputting Frequency to fs");
}

void drawDisplay()
{
  Serial.println("Entry - [drawDisplay]");
  
/* Selector switch has moved off XPDR */
  if (bWasXPDR)
  { // the switch has moved from XPDR to something else, therefore need to send the XPDR frequency.
    Serial.println("was XPDR");
    max7219Control.displayNumber(DEVICEID_TRANSPONDER, allRadios[XPDR].stbyFrequency, 5, 4); //REQUIRES LEADING ZERO, another reason to perhaps have displayNumber as a string rather than numbers�
    Serial.println("send XPDR stbyFrequency");
    bWasXPDR = false;
  }
  else {  }

switch (activeRadio)
  {
    case NONE:
      {
        Serial.println("### NO RADIO SELECTED ###");
        break;
      }
    case XPDR:
      {
        max7219Control.displayNumber(DEVICEID_TRANSPONDER, allRadios[XPDR].stbyFrequency, 5, 4, 4 - activeXPDRDigit, 1, true, true); //want to draw a decimal point on the activeXPDRDigit
        break;
      }
    case ADF:
      {
        max7219Control.blankDisplay(DEVICEID_ACTIVEFREQUENCY); //blank the ACTIVEFREQUENCY display as it's not used
        max7219Control.blankDisplay(DEVICEID_STANDBYFREQUENCY); //blank the STANDBYFREQUENCY prior to drawing the ADF
        max7219Control.displayNumber(DEVICEID_STANDBYFREQUENCY, allRadios[ADF].stbyFrequency, 6, 3, 3 - activeADFDigit, 1, true, true); //want to draw a decimal point on the activeADFDigit
        // NOTE that this doesn't need the bWasADF (as with the XPDR) as one of the other radios will overwrite it.
        break;
      }
    default : //for all other radios, the frequencies of the active one are displayed
      {
        max7219Control.displayNumber(DEVICEID_ACTIVEFREQUENCY, allRadios[activeRadio].activeFrequency, 1, 6, 3);
        max7219Control.displayNumber(DEVICEID_STANDBYFREQUENCY, allRadios[activeRadio].stbyFrequency, 3, 6, 3); //use 3rd position to shift by 2 to the right
        break;
      }
  }
  Serial.println("Exit - [drawDisplay].");
}

/*
  ### coerceFrequency ### : returns a coerced frequency depending upon the radio selected.
  bIncrement : whether its incrementing (TRUE) or decrementing (FALSE)
*/
long coerceFrequency(bool bIncrement)
{
  Serial.println("Entry - [coerceFrequency]");
  bool bMajor = radioStackSwitches.isHIGH(RS_MAJOR_FREQUENCY_SWITCH);
  long tempMajor = allRadios[activeRadio].stbyFrequency / 1000;
  long tempMinor = allRadios[activeRadio].stbyFrequency - tempMajor * 1000;

  switch (activeRadio)
  {
    case XPDR:
      {
        // could call a generic function which handles various numbers of field i.e. ADF will be the same code, but with 3 digits instead of 4
        // depends upon which digit is changing to how it wraps around�
        // work on each digit independently
        //split the frequency into 4 digits
        int tempFrequency[4];
        long tempFrequency2 = allRadios[XPDR].stbyFrequency;
        for (int i = 0; i < 4; i++)
        { // build the frequency one digit at a time
          tempFrequency[i] = tempFrequency2 / pow(10, (4 - (i + 1)));
          tempFrequency2 -= tempFrequency[i] * pow(10, (4 - (i + 1)));
        }
        //work on which one is changing.
        if (bIncrement)
        {
          if ((tempFrequency[activeXPDRDigit] + 1) > 9) tempFrequency[activeXPDRDigit] = 0;
          else tempFrequency[activeXPDRDigit] += 1;
        }
        else //decrementing
        {
          if ((tempFrequency[activeXPDRDigit] - 1) < 0) tempFrequency[activeXPDRDigit] = 9;
          else tempFrequency[activeXPDRDigit] -= 1;
        }

        //rebuild value
        tempMinor = 0;
        for (int i = 0; i < 4; i++)
        { // rebuild the frequency one digit at a time
          tempMinor += tempFrequency[i] * pow(10, (4 - (i + 1)));
        }
        tempMajor = 0;
        break;
      }
    case ADF:
      {
        //split the frequency into 3 digits
        int tempFrequency[3];
        long tempFrequency2 = allRadios[ADF].stbyFrequency;
        for (int i = 0; i < 3; i++)
        { // build the frequency one digit at a time
          tempFrequency[i] = tempFrequency2 / pow(10, (3 - (i + 1)));
          tempFrequency2 -= tempFrequency[i] * pow(10, (3 - (i + 1)));
        }
        //work on which one is changing.
        if (bIncrement)
        {
          if ((tempFrequency[activeADFDigit] + 1) > 9) tempFrequency[activeADFDigit] = 0;
          else tempFrequency[activeADFRDigit] += 1;
        }
        else //decrementing
        {
          if ((tempFrequency[activeADFDigit] - 1) < 0) tempFrequency[activeADFDigit] = 9;
          else tempFrequency[activeADFDigit] -= 1;
        }

        //rebuild value
        tempMinor = 0;
        for (int i = 0; i < 3; i++)
        { // rebuild the frequency one digit at a time
          tempMinor += tempFrequency[i] * pow(10, (3 - (i + 1)));
        }
        tempMajor = 0;
        break;
      }
    default:
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
Serial.print("[coerceFrequency] tempMajor : ");
Serial.println(tempMajor *1000);
Serial.print("[coerceFrequency] tempMinor : ");
Serial.println(tempMinor);
Serial.println(tempMajor * 1000 + tempMinor);
Serial.println("Exit - [coerceFrequency].");
  return (tempMajor * 1000 + tempMinor);
}

void RADIOSTACK_READ()
{
  Serial.println("Entry - [RADIOSTACK_READ]");

  int iCodeIn;
  for (int i = 0; i < 500; i++)
  {
    Serial.print("i=");
    Serial.println(i);
    if (Serial.available())// && (!dataInitialised)) //checks for data on the bus and that the data hasn't hasn't already been read.
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
  }
  Serial.print(dataInitialised);
  Serial.println("Exit - [RADIOSTACK_READ].");
}

// reads the frequency from the string fs string
long readFrequency(eRadios fRadio)
{
  long tempFrequency;
  if (fRadio == XPDR)
  {
    Serial.println("XPDR READ");
    //length will be 4 characters
    tempFrequency = commonFunction.getLong(4);
  }
  else if (fRadio == ADF)
  {
    Serial.println("ADF READ");
    //length will be 4 characters
    tempFrequency = commonFunction.getLong(4, 1);
  }
  else if ((fRadio == COMM1) || (fRadio == COMM2))
  {
    //length will be 6 characters
    tempFrequency = commonFunction.getLong(7, 3);
  }
  else if ((fRadio == NAV1) || (fRadio == NAV2) // all other radios will be 6 digits
  {
    //length will be 6 characters
    tempFrequency = commonFunction.getLong(6, 2);
  }
  else
  {
    Serial.println("### ILLEGAL CALL - Not a valid Radio Selected [readFrequency] ###");
  }

  return (tempFrequency);
}

            }
        }
      }
    }
  }
  Serial.print(dataInitialised);
  Serial.println("Exit - [RADIOSTACK_READ].");
}

// reads the frequency from the string fs string
