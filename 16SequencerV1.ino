//Code by Fihdi 2026

#include "TCA9555.h"

//Arduino Nano Pins

#define PLAY 2     //Pause and Play input
#define DIR 3      //Direction input
#define RST 4      //Reset input
#define CLK 5      //Clock input
#define LENGTH A1  //Length Input
#define CVFB A2

#define PROGA 11  //Button Input for programming the A-Gates
#define PROGB 8   //Button Input for programming the B-Gates
#define PROGC 13  //Button Input for programming the C-Gates
#define PROGD A3  //Button Input for programming the D-Gates

#define GATEA 6   //GATE A output
#define GATEB 7   //GATE B output
#define GATEC 12  //GATE C output
#define GATED 9   //GATE D output
#define GATEP 10  //Probability Gate Output

//LED and Pushbutton IO Expander Pins

TCA9555 LEDPUSHEXPANDER(0x20);

TCA9555 POTEXPANDER(0x21);

#define LEDCOL1 7
#define LEDCOL2 6
#define LEDCOL3 5
#define LEDCOL4 4
#define LEDROW1 0
#define LEDROW2 1
#define LEDROW3 2
#define LEDROW4 3

#define PUSHROW1 8
#define PUSHROW2 9
#define PUSHROW3 10
#define PUSHROW4 11
#define PUSHCOL1 12
#define PUSHCOL2 13
#define PUSHCOL3 14
#define PUSHCOL4 15

#define CLK 5

int length = 16;
int lastCV = 0;        // last value from the length potentiometer
long timemarkLength = 0;   //timemark until the length should stay displayed

long timemarkCVFB = 0;    // timemark to read in CVFB after Clock signal
bool CVFBread = false;    // boolean if the CVFB has been read in yet after a CLK

int LEDMatrixCounter = 0;  // Counter to display the LEDs
int stepCounter = 0;       //Current step
int lastStep = 0;          //The step before the current step

int progMode = 0;              //Programming Mode (0 = no Mode(show current step); 1 = Programming A Gates; 2 = Programming B Gates; 3 = Programming C Gates; 4 = Programming D Gates)
bool ProgInterrupted = false;  // boolean if a GATE programming button has been pushed

bool CLKInterrupted = false;     // boolean for the Clock ISR
bool RSTInterrupted = false;     // boolean for the Reset ISR
bool DIRInterrupted = false;     // boolean for the Direction ISR
bool PLAYInterrupted = false;    //boolean for the Pause / Play ISR
bool KeyPadInterrupted = false;  //boolean for the Keypad ISR

bool isPlaying = true;
int direction = 0;  //0 = forward, 1 = backwards, 2 = random,

bool GatesA[16];
bool GatesB[16];
bool GatesC[16];
bool GatesD[16];

bool LEDmatrix[16];

void setup() {
  Wire.begin();
  LEDPUSHEXPANDER.begin();
  POTEXPANDER.begin();
  Wire.setClock(50000);

  //Set the Potentiometer Pins as outputs and set them low at startup.
  for (int i = 0; i < 16; i++) {
    POTEXPANDER.pinMode1(i, OUTPUT);
    POTEXPANDER.write1(i, LOW);
  }

  //Set the LED Matrix pins as outputs
  for (int i = 0; i < 8; i++) {
    LEDPUSHEXPANDER.pinMode1(i, OUTPUT);
  }

  LEDPUSHEXPANDER.pinMode1(PUSHCOL1, OUTPUT);
  LEDPUSHEXPANDER.pinMode1(PUSHCOL2, OUTPUT);
  LEDPUSHEXPANDER.pinMode1(PUSHCOL3, OUTPUT);
  LEDPUSHEXPANDER.pinMode1(PUSHCOL4, OUTPUT);
  LEDPUSHEXPANDER.pinMode1(PUSHROW1, INPUT);
  LEDPUSHEXPANDER.pinMode1(PUSHROW2, INPUT);
  LEDPUSHEXPANDER.pinMode1(PUSHROW3, INPUT);
  LEDPUSHEXPANDER.pinMode1(PUSHROW4, INPUT);

  pinMode(PROGA, INPUT_PULLUP);
  pinMode(PROGB, INPUT_PULLUP);
  pinMode(PROGC, INPUT_PULLUP);
  pinMode(PROGD, INPUT_PULLUP);

  pinMode(GATEA, OUTPUT);
  pinMode(GATEB, OUTPUT);
  pinMode(GATEC, OUTPUT);
  pinMode(GATED, OUTPUT);
  pinMode(GATEP, OUTPUT);

  pinMode(CLK, INPUT_PULLUP);
  pinMode(RST, INPUT_PULLUP);
  pinMode(DIR, INPUT_PULLUP);
  pinMode(PLAY, INPUT_PULLUP);
  pinMode(LENGTH, INPUT);

  LEDPUSHEXPANDER.write1(LEDROW1, HIGH);
  LEDPUSHEXPANDER.write1(LEDROW2, HIGH);
  LEDPUSHEXPANDER.write1(LEDROW3, HIGH);
  LEDPUSHEXPANDER.write1(LEDROW4, HIGH);
}

void loop() {

  checkRST();

  checkLengthChange();

  checkPLAY();

  if (isPlaying) {
    checkCLK();
    checkDIR();
  }
  
  if((millis() > timemarkCVFB) && CVFBread==false){
    //Output the probability gate after the time mark (to not read in the value durring a transistion)
    CVFBread=true;

    //Higher voltage on the pin means a higher chance for the P-Gate to turn on
    if( random(1000) < analogRead(CVFB) ){
      digitalWrite(GATEP, HIGH);
    }
  }

  checkProg();

  if (progMode != 0) {
    checkKeyPad();
  }

  displayMatrix();
}

void displayMatrix() {

  for (int i = 0; i < 4; i++) {
    LEDPUSHEXPANDER.write1(LEDROW1, HIGH);
    LEDPUSHEXPANDER.write1(LEDROW2, HIGH);
    LEDPUSHEXPANDER.write1(LEDROW3, HIGH);
    LEDPUSHEXPANDER.write1(LEDROW4, HIGH);

    if (i == 0) {
      //Update COL1

      LEDPUSHEXPANDER.write1(LEDCOL1, HIGH);
      LEDPUSHEXPANDER.write1(LEDCOL2, LOW);
      LEDPUSHEXPANDER.write1(LEDCOL3, LOW);
      LEDPUSHEXPANDER.write1(LEDCOL4, LOW);

      if (LEDmatrix[0]) { LEDPUSHEXPANDER.write1(LEDROW1, LOW); }   //ROW1
      if (LEDmatrix[4]) { LEDPUSHEXPANDER.write1(LEDROW2, LOW); }   //ROW2
      if (LEDmatrix[8]) { LEDPUSHEXPANDER.write1(LEDROW3, LOW); }   //ROW3
      if (LEDmatrix[12]) { LEDPUSHEXPANDER.write1(LEDROW4, LOW); }  //ROW4
    }

    if (i == 1) {
      //Update COL2
      LEDPUSHEXPANDER.write1(LEDCOL1, LOW);
      LEDPUSHEXPANDER.write1(LEDCOL2, HIGH);
      LEDPUSHEXPANDER.write1(LEDCOL3, LOW);
      LEDPUSHEXPANDER.write1(LEDCOL4, LOW);

      if (LEDmatrix[1]) { LEDPUSHEXPANDER.write1(LEDROW1, LOW); }   //ROW1
      if (LEDmatrix[5]) { LEDPUSHEXPANDER.write1(LEDROW2, LOW); }   //ROW2
      if (LEDmatrix[9]) { LEDPUSHEXPANDER.write1(LEDROW3, LOW); }   //ROW3
      if (LEDmatrix[13]) { LEDPUSHEXPANDER.write1(LEDROW4, LOW); }  //ROW4
    }

    if (i == 2) {
      //Update COL3
      LEDPUSHEXPANDER.write1(LEDCOL1, LOW);
      LEDPUSHEXPANDER.write1(LEDCOL2, LOW);
      LEDPUSHEXPANDER.write1(LEDCOL3, HIGH);
      LEDPUSHEXPANDER.write1(LEDCOL4, LOW);

      if (LEDmatrix[2]) { LEDPUSHEXPANDER.write1(LEDROW1, LOW); }   //ROW1
      if (LEDmatrix[6]) { LEDPUSHEXPANDER.write1(LEDROW2, LOW); }   //ROW2
      if (LEDmatrix[10]) { LEDPUSHEXPANDER.write1(LEDROW3, LOW); }  //ROW3
      if (LEDmatrix[14]) { LEDPUSHEXPANDER.write1(LEDROW4, LOW); }  //ROW4
    }
    if (i == 3) {
      //Update COL3
      LEDPUSHEXPANDER.write1(LEDCOL1, LOW);
      LEDPUSHEXPANDER.write1(LEDCOL2, LOW);
      LEDPUSHEXPANDER.write1(LEDCOL3, LOW);
      LEDPUSHEXPANDER.write1(LEDCOL4, HIGH);

      if (LEDmatrix[3]) { LEDPUSHEXPANDER.write1(LEDROW1, LOW); }   //ROW1
      if (LEDmatrix[7]) { LEDPUSHEXPANDER.write1(LEDROW2, LOW); }   //ROW2
      if (LEDmatrix[11]) { LEDPUSHEXPANDER.write1(LEDROW3, LOW); }  //ROW3
      if (LEDmatrix[15]) { LEDPUSHEXPANDER.write1(LEDROW4, LOW); }  //ROW4
    }
  }
}

void updateLEDMatrix() {

  //Show the length OR the gates and the current step
  if (millis() < timemarkLength) {
    for (int i = 0; i < 16; i++) {
      LEDmatrix[i] = (i < length);
    }
  } else {

    for (int i = 0; i < length; i++) {

      if (progMode == 0) {
        //Step mode
        LEDmatrix[i] = (i == stepCounter);
      }

      if (progMode == 1) {
        //Load the A Gates into the LED Matrix
        LEDmatrix[i] = GatesA[i];  //Set the current step (StepCounter) and the corresponding LED in the Matrix
      }
      if (progMode == 2) {
        //Load the B Gates into the LED Matrix
        LEDmatrix[i] = GatesB[i];  //Set the current step (StepCounter) and the corresponding LED in the Matrix
      }
      if (progMode == 3) {
        //Load the C Gates into the LED Matrix
        LEDmatrix[i] = GatesC[i];  //Set the current step (StepCounter) and the corresponding LED in the Matrix
      }
      if (progMode == 4) {
        //Load the D Gates into the LED Matrix
        LEDmatrix[i] = GatesD[i];  //Set the current step (StepCounter) and the corresponding LED in the Matrix
      }
    }
  }
}

void checkCLK() {
  // Checks if the clock input has a rising edge
  //Updates the step counter Gate outputs and CV output

  if ((digitalRead(CLK) == LOW) && (CLKInterrupted == false)) {
    //Clock has been triggered, change the step according to the direction setting
    CLKInterrupted = true;

    //0 = forward, 1 = backwards, 2 = random,
    if (direction == 0) {
      stepCounter = (stepCounter + 1) % length;
    } else if (direction == 1) {
      stepCounter = (stepCounter - 1 + length) % length;
    } else if (direction == 2) {
      stepCounter = random(length);
    }

    //Turn the Gates ON:

    if (GatesA[stepCounter]) {
      digitalWrite(GATEA, HIGH);
    }
    if (GatesB[stepCounter]) {
      digitalWrite(GATEB, HIGH);
    }
    if (GatesC[stepCounter]) {
      digitalWrite(GATEC, HIGH);
    }
    if (GatesD[stepCounter]) {
      digitalWrite(GATED, HIGH);
    }

    //Turn the last step off and the new step (stepCounter) on
    POTEXPANDER.write1(lastStep, LOW);
    POTEXPANDER.write1(stepCounter, HIGH);

    CVFBread = false;
    timemarkCVFB = millis() + 10; // read in the CVFB 10ms after the new step

    if (progMode == 0) {
      updateLEDMatrix();
    }
    
  }

  if ((digitalRead(CLK) == HIGH) && (CLKInterrupted == true)) {
    CLKInterrupted = false;  //Reset trigger flag

    //Turn the Gates OFF
    digitalWrite(GATEA, LOW);
    digitalWrite(GATEB, LOW);
    digitalWrite(GATEC, LOW);
    digitalWrite(GATED, LOW);
    digitalWrite(GATEP, LOW);
    //Reset the CVFB flag for the next 
    
    lastStep = stepCounter;
  }
}

void checkLengthChange() {
  //Checks for updates on the length potentiometer
  int raw = analogRead(LENGTH);  // 0–1023
  
  //Hysteresis check
  if( ( (lastCV-raw) > 30) || ( (raw-lastCV) > 30) ){

    lastCV = raw;
    //Mapping (900 is the upper bound in case potentiometer does not reach the full 1023 range)

    length = map(raw, 0, 900, 1, 16);
    length = constrain(length, 1, 16);

    //Show the length on the LED Matrix until timemarkLength. 
    timemarkLength = millis() + 1000;

  }

}

void checkPLAY() {
  if ((digitalRead(PLAY) == LOW) && (PLAYInterrupted == false)) {
    //Reset the stepCounter and jump back to step 1.
    PLAYInterrupted = true;

    isPlaying = !isPlaying;
  }

  if ((digitalRead(PLAY) == HIGH) && (PLAYInterrupted == true)) {
    PLAYInterrupted = false;  //Reset trigger flag
  }
}

void checkDIR() {
  if ((digitalRead(DIR) == LOW) && (DIRInterrupted == false)) {
    //Change the direction mode
    DIRInterrupted = true;
    direction = (direction + 1) & 3;
  }

  if ((digitalRead(DIR) == HIGH) && (DIRInterrupted == true)) {
    DIRInterrupted = false;  //Reset trigger flag
  }
}

void checkRST() {
  if ((digitalRead(RST) == LOW) && (RSTInterrupted == false)) {
    //Reset the stepCounter and jump back to step 1.
    //Turn the last step and the current step off
    POTEXPANDER.write1(lastStep, LOW); //turn the last step off
    POTEXPANDER.write1(stepCounter, LOW); //turn the current step off
    RSTInterrupted = true;
    stepCounter = 0;
    POTEXPANDER.write1(stepCounter, HIGH); //turn the current step (step 1) ON
  }

  if ((digitalRead(RST) == HIGH) && (RSTInterrupted == true)) {
    RSTInterrupted = false;  //Reset trigger flag
  }
}

void checkProg() {
  if (((digitalRead(PROGA) == LOW) || (digitalRead(PROGB) == LOW) || (digitalRead(PROGC) == LOW) || (digitalRead(PROGD) == LOW)) && (ProgInterrupted == false)) {
    //A programming button has been triggered
    ProgInterrupted = true;


    if (digitalRead(PROGA) == LOW) {
      digitalWrite(GATEA, HIGH);
      progMode = 1;
    }
    if (digitalRead(PROGB) == LOW) {
      digitalWrite(GATEB, HIGH);
      progMode = 2;
    }

    if (digitalRead(PROGC) == LOW) {
      digitalWrite(GATEC, HIGH);
      progMode = 3;
    }

    if (digitalRead(PROGD) == LOW) {
      digitalWrite(GATED, HIGH);
      progMode = 4;
    }
    updateLEDMatrix();
  }
  if ((digitalRead(PROGA) == HIGH) && (digitalRead(PROGB) == HIGH) && (digitalRead(PROGC) == HIGH) && (digitalRead(PROGD) == HIGH) && (ProgInterrupted == true)) {
    //All programming buttons have been let go and the ISR has finished
    digitalWrite(GATEA, LOW);
    digitalWrite(GATEB, LOW);
    digitalWrite(GATEC, LOW);
    digitalWrite(GATED, LOW);

    ProgInterrupted = false;  //Reset trigger flag
    progMode = 0;
    updateLEDMatrix();
  }
}

void checkKeyPad() {

  int keypad = readKeyPad();

  if ((keypad != 16) && (KeyPadInterrupted == false)) {
    KeyPadInterrupted = true;
    //A button on the Keypad has been pressed. Activate or Deactivate the corresponding Gate

    if (progMode == 1) {
      GatesA[keypad] = !GatesA[keypad];
    }
    if (progMode == 2) {
      GatesB[keypad] = !GatesB[keypad];
    }
    if (progMode == 3) {
      GatesC[keypad] = !GatesC[keypad];
    }
    if (progMode == 4) {
      GatesD[keypad] = !GatesD[keypad];
    }

    updateLEDMatrix();
  }
  if ((keypad == 16) && (KeyPadInterrupted == true)) {
    KeyPadInterrupted = false;
  }
}

int readKeyPad() {
  /*    
return table:
      | COL 1 | COL2 | COL3 | COL4 |
  ROW1|   0   |  1   |  2   |  3   |
  ROW2|   4   |  5   |  6   |  7   |
  ROW3|   8   |  9   |  10  |  11  |
  ROW4|   12  |  13  |  14  |  15  |
*/

  //Check Column 1:
  LEDPUSHEXPANDER.write1(PUSHCOL1, HIGH);
  LEDPUSHEXPANDER.write1(PUSHCOL2, LOW);
  LEDPUSHEXPANDER.write1(PUSHCOL3, LOW);
  LEDPUSHEXPANDER.write1(PUSHCOL4, LOW);

  if (LEDPUSHEXPANDER.read1(PUSHROW1) == HIGH) { return 0; }
  if (LEDPUSHEXPANDER.read1(PUSHROW2) == HIGH) { return 4; }
  if (LEDPUSHEXPANDER.read1(PUSHROW3) == HIGH) { return 8; }
  if (LEDPUSHEXPANDER.read1(PUSHROW4) == HIGH) { return 12; }

  //Check Column 2:
  LEDPUSHEXPANDER.write1(PUSHCOL1, LOW);
  LEDPUSHEXPANDER.write1(PUSHCOL2, HIGH);
  LEDPUSHEXPANDER.write1(PUSHCOL3, LOW);
  LEDPUSHEXPANDER.write1(PUSHCOL4, LOW);

  if (LEDPUSHEXPANDER.read1(PUSHROW1) == HIGH) { return 1; }
  if (LEDPUSHEXPANDER.read1(PUSHROW2) == HIGH) { return 5; }
  if (LEDPUSHEXPANDER.read1(PUSHROW3) == HIGH) { return 9; }
  if (LEDPUSHEXPANDER.read1(PUSHROW4) == HIGH) { return 13; }

  //Check Column 3:
  LEDPUSHEXPANDER.write1(PUSHCOL1, LOW);
  LEDPUSHEXPANDER.write1(PUSHCOL2, LOW);
  LEDPUSHEXPANDER.write1(PUSHCOL3, HIGH);
  LEDPUSHEXPANDER.write1(PUSHCOL4, LOW);

  if (LEDPUSHEXPANDER.read1(PUSHROW1) == HIGH) { return 2; }
  if (LEDPUSHEXPANDER.read1(PUSHROW2) == HIGH) { return 6; }
  if (LEDPUSHEXPANDER.read1(PUSHROW3) == HIGH) { return 10; }
  if (LEDPUSHEXPANDER.read1(PUSHROW4) == HIGH) { return 14; }

  //Check Column 4:
  LEDPUSHEXPANDER.write1(PUSHCOL1, LOW);
  LEDPUSHEXPANDER.write1(PUSHCOL2, LOW);
  LEDPUSHEXPANDER.write1(PUSHCOL3, LOW);
  LEDPUSHEXPANDER.write1(PUSHCOL4, HIGH);

  if (LEDPUSHEXPANDER.read1(PUSHROW1) == HIGH) { return 3; }
  if (LEDPUSHEXPANDER.read1(PUSHROW2) == HIGH) { return 7; }
  if (LEDPUSHEXPANDER.read1(PUSHROW3) == HIGH) { return 11; }
  if (LEDPUSHEXPANDER.read1(PUSHROW4) == HIGH) { return 15; }

  return 16;  //No Key Pressed
}
