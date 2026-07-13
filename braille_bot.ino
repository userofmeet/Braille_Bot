#include <LiquidCrystal_I2C.h>
#include <mechButton.h>
#include <Servo.h>
#include <serialStr.h>
#include <resizeBuff.h>
#include <autoPOT.h>
#include <mapper.h>
#include "braille_symbols.h"

#define MIN_MS 750
#define MAX_MS 2000

const int servoPins[] = {10, 5, 9, 6, 8, 7};
int asciiValue;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo[6];
mechButton repeatBtn(2);
mechButton backBtn(3);
mechButton fwdBtn(4);
serialStr serialMgr;
char* aStr;
int strIndex;
timeObj stringTimer(1000, false);
autoPOT potMgr(A0);
mapper potMapper(0, 1023, MAX_MS, MIN_MS);
float msPerChar;
bool newDelay;

void backBtnClk(void) 
{
  if (!backBtn.getState()) 
  {
    if (asciiValue > 32) 
    {
      asciiValue--;
    }
    showChar(asciiValue);
  }
}

void fwdBtnClk(void) 
{
  if (!fwdBtn.getState()) 
  {
    if (asciiValue < 90) 
    {
      asciiValue++;
    }
    showChar(asciiValue);
  }
}

void recievedStr(char* inStr) 
{
  int numBytes;
  Serial.println(inStr);
  if (aStr) 
  {
    Serial.println("Sorry, already reading a string");
    return;
  } 
  else 
  {
    numBytes = strlen(inStr);
    numBytes++;
    if (resizeBuff(numBytes, &aStr)) 
    {
      for (int i = 0; i < numBytes - 1; i++) 
      {
        aStr[i] = toupper(inStr[i]);
      }
      aStr[numBytes - 1] = '\0';
      strIndex = 0;
      showChar((int)(aStr[strIndex]));
      stringTimer.start();
    }
  }
}

void newValue(int inVal)
{
  msPerChar = potMapper.map(inVal);
  newDelay = true;
}

void moveServos(int value)  
{
  byte moveByte = symbols[value - 32];
  if (moveByte & 128)  
  {
    for (int idx = 0; idx <= 5; idx++)  
    {
      servo[idx].write(90);
    }
  } 
  else  
  {
    moveByte & 0b00100000 ? servo[0].write(75) : servo[0].write(115);
    moveByte & 0b00010000 ? servo[1].write(115) : servo[1].write(75);
    moveByte & 0b00001000 ? servo[2].write(75) : servo[2].write(115);
    moveByte & 0b00000100 ? servo[3].write(115) : servo[3].write(75);
    moveByte & 0b00000010 ? servo[4].write(75) : servo[4].write(115);
    moveByte & 0b00000001 ? servo[5].write(115) : servo[5].write(75);
  }
}

void showChar(int charCode) 
{
  char myChar = (char)charCode;
  byte moveByte = symbols[charCode - 32];
  lcd.setCursor(12, 0);
  lcd.print(myChar);
  lcd.setCursor(10, 1);
  if (moveByte & 128)  
  {
  
  } 
  else 
  {
    for (int bit = 5; bit >= 0; bit--)  
    {
      lcd.print(bitRead(moveByte, bit));
    }
  }
  moveServos(charCode);
}

void setup() 
{
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  for (int idx = 0; idx <= 5; idx++)  
  {
    servo[idx].attach(servoPins[idx]);
  }
  fwdBtn.setCallback(fwdBtnClk);
  backBtn.setCallback(backBtnClk);
  repeatBtn.setCallback(showChar);
  serialMgr.setCallback(recievedStr);
  potMgr.setCallback(newValue);
  aStr = NULL;                            // Plan on using resizeBuff() on this. Need to start at NULL.
  newDelay = false;

  // splash screen
  lcd.setCursor(4, 0);
  lcd.print("Braille");
  lcd.setCursor(1, 1);
  lcd.print("Trainer v1.3.0");
  delay(1000);
  lcd.clear();
  // initialize
  lcd.setCursor(0, 0);
  lcd.print("Character :");
  lcd.setCursor(0, 1);
  lcd.print("Braille :");
  asciiValue = 65;  // A
  showChar(asciiValue);

  Serial.println("***   Type a string to be read.  ***");
  Serial.println("Use the knob to adjust reading speed.");        // To adjust the speed of the braille while reading the text
}

void loop() 
{
  idle();                                 // Let background tasks, like the buttons, run.
  if (aStr) 
  {                             // If aStr is non NULL..
    if (stringTimer.ding()) {              // If the string timer has expired..
      strIndex++;                         // Bmp up our string reading index.
      if (aStr[strIndex] == '\0') {        
       // If we're looking at the end char..
        resizeBuff(0,&aStr);              // recycle the string memory.
        //aStr = NULL;                    // Set the string pointer to NULL as a flag.
        stringTimer.reset();              // Reset our timer.
      } else {                            // Else, we're NOT pointing at end of string.
        showChar((int)(aStr[strIndex]));  // Show this char.
        if (newDelay) {                   // If the timer POT changed..
          stringTimer.setTime(msPerChar); // Set the new time into the string timer.
          newDelay = false;               // Clear the flag.
        } else {                          // Else, we're using the old time..
          stringTimer.stepTime();         // Restart the timer.
        }
      }
    }
  }
}
