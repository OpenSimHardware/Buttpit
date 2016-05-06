#include "LedControl.h"
#include <rotary.h>
#include <Keyboard.h>


// PINS for LED
#define DINPIN 10     //PIN to DIN
#define CSPIN 16      //PIN to CS
#define CLKPIN 14     //PIN to CLK

// 1st encoder - for fuel
#define ROTARY_PIN1 3
#define ROTARY_PIN2 2
// 2nd encoder - for next/prev control
#define ROTARY_PIN3 0
#define ROTARY_PIN4 1
// 3rd encoder - for change value
#define ROTARY_PIN5 4
#define ROTARY_PIN6 5
// define to enable weak pullups.
#define ENABLE_PULLUPS


#define NumSwitches 4
#define ShortPressTime 200
#define ShowPressuresSwitchTime 2000

#define FT B00100010  // BOTH FRONT TIRES
#define RT B00010100  // BOTH REAR TYRES
#define WDS B00000001  // WINDSCREEN
#define FRP B01000000  // FAST REPAIR


#define KeysPin1 A3
#define KeysPin2 A2
#define NumAnalogWires 2

int FrontTiresPress=0, RearTiresPress=0; 
long TimeTmp;//,TTmpKeys;
boolean ShowPressuresFlag = 0;
boolean ChangeFlag = 0;


struct AnalogKeysStruct {
  int PIN;
  int AnalogKeyValue;
  int PrevAnalogKeyValue;
  long TTmpKeys; 
};
  
AnalogKeysStruct buttons[NumAnalogWires] = {
  {KeysPin1, 0, 0, 0},
  {KeysPin2, 0, 0, 0},
};


Rotary r1 = Rotary(ROTARY_PIN1, ROTARY_PIN2);
Rotary r2 = Rotary(ROTARY_PIN3, ROTARY_PIN4);
Rotary r3 = Rotary(ROTARY_PIN5, ROTARY_PIN6);


volatile unsigned int encoderPos = 0;  // a counter for the dial


struct SwitchStates {   // struct's def for holding states of our switches
  int PIN;              // switch's PIN
  boolean State;        
  boolean PrevState;
  long TimePressed; 
  boolean ShortPress;
};

SwitchStates Switches[] = {
    {6,false,false,0,false},
    {7,false,false,0,false},
    {8,false,false,0,false},
    {9,false,false,0,false},
};



byte PitCombo,PrevPitCombo = 0;


LedControl lc=LedControl(DINPIN,CLKPIN,CSPIN,1);




void setup() {
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  /* and clear the display */
  lc.clearDisplay(0);
  
  for (byte i=0;i<NumSwitches;i++) {
    pinMode(Switches[i].PIN, INPUT_PULLUP);
  }
  
  pinMode(ROTARY_PIN1, INPUT_PULLUP); 
  pinMode(ROTARY_PIN2, INPUT_PULLUP); 
  pinMode(ROTARY_PIN3, INPUT_PULLUP);
  pinMode(ROTARY_PIN4, INPUT_PULLUP);
  pinMode(ROTARY_PIN5, INPUT_PULLUP);
  pinMode(ROTARY_PIN6, INPUT_PULLUP);
  
  Keyboard.begin();
  
  TimeTmp = millis();
}



void loop() { 
  
  CheckSwitches(); //check current switches' states and populate structure
  
  ProcessSwitches(); //process switches' structure and show corresponding pictograph

  CheckFuelEncoder(); //check position of fuel encoder and show requested number of fuel
  
  CheckControlEncoder(); //check and process control encoder
  
  CheckChangeEncoder(); //check and process change encoder 
  
  CheckButtons();
  
  ShowPressures();
  
}


void ProcessPitCombo(void) {
  String StrResult = "#clear";
  String tmp;
  Keyboard.write('t');
  delay(500);
//  Keyboard.print("#clear");
//  delay (50);
  if (FrontTiresPress) {
    FrontTiresPress > 0 ? tmp = "+" : tmp = "";
    StrResult = String(StrResult + " lf " + tmp + FrontTiresPress + " rf " + tmp + FrontTiresPress);
  }
  if (RearTiresPress) {
    RearTiresPress > 0 ? tmp = "+" : tmp = "";
    StrResult = String(StrResult + " lr " + tmp + RearTiresPress + " rr " + tmp + RearTiresPress);
  }
  if (PitCombo) {
    if (PitCombo & FT && !FrontTiresPress) {
      StrResult = String(StrResult + " lf rf");
    }
    if (PitCombo & RT && !RearTiresPress) {
      StrResult = String(StrResult + " lr rr");
    }
  }
  if (encoderPos) {
    StrResult = String(StrResult + " fuel " + encoderPos);
  }
  if (PitCombo) {
    if (PitCombo & FRP) {
      StrResult = String(StrResult + " fr");
    }
    if (PitCombo & WDS) {
      StrResult = String(StrResult + " ws");
    }
  }
  
//  StrResult = String(StrResult + "$");
  Keyboard.print(StrResult);
  delay(ShortPressTime);
  Keyboard.write(KEY_RETURN);
//  delay(ShortPressTime);
}


void ShowPressures(void) {
  if ((millis() - TimeTmp) > ShowPressuresSwitchTime) {
    TimeTmp = millis();
    ShowPressuresFlag = ShowPressuresFlag ^ 1;
  }
  
  if ((FrontTiresPress != 0) && (RearTiresPress == FrontTiresPress)) {
      ShowNumber(RearTiresPress,4);
      lc.setRow(0,7,RT|FT);
      return;
    } 
    
  if (ShowPressuresFlag && FrontTiresPress) {
      ShowNumber(FrontTiresPress,4);
      lc.setRow(0,7,FT);
      ChangeFlag = 0;
      return;
    }
    
  if (!ShowPressuresFlag && RearTiresPress) {
      ShowNumber(RearTiresPress,4);
      lc.setRow(0,7,RT);
      ChangeFlag = 0;
      return;
    }
    
  if ((FrontTiresPress == 0) && (RearTiresPress == FrontTiresPress)) {
      //ShowNumber(RearTiresPress,4);
      lc.setRow(0,7,0);
      lc.setRow(0,6,0);
      lc.setRow(0,5,0);
      lc.setRow(0,4,0);
      return;
    }  
}

void CheckButtons(void) {
  for (int i=0;i<NumAnalogWires;i++) {
  buttons[i].AnalogKeyValue = analogRead(buttons[i].PIN);
  
  if (buttons[i].AnalogKeyValue>150 && buttons[i].PrevAnalogKeyValue>150 && (millis() - buttons[i].TTmpKeys) > ShortPressTime) {
    buttons[i].TTmpKeys = millis();
    if (buttons[i].AnalogKeyValue > 600 && buttons[i].PrevAnalogKeyValue > 600) {
      if (!i) FrontTiresPress --; ShowPressuresFlag=1;
      if (i) Keyboard.write(KEY_ESC);
      delay(ShortPressTime);
    }
    if (buttons[i].AnalogKeyValue > 350 && buttons[i].AnalogKeyValue <= 600 && buttons[i].PrevAnalogKeyValue > 350 && buttons[i].PrevAnalogKeyValue <= 600) {
      if (!i) FrontTiresPress ++; ShowPressuresFlag=1;
      if (i) Keyboard.write(KEY_TAB);
      delay(ShortPressTime);
    }
    if (buttons[i].AnalogKeyValue > 250 && buttons[i].AnalogKeyValue <= 350 && buttons[i].PrevAnalogKeyValue > 250 && buttons[i].PrevAnalogKeyValue <= 350) {
       if (!i) RearTiresPress --; ShowPressuresFlag=0;
       if (i) Keyboard.write(0x20);
       delay(ShortPressTime);
    }
    if (buttons[i].AnalogKeyValue > 200 && buttons[i].AnalogKeyValue <= 250 && buttons[i].PrevAnalogKeyValue > 200 && buttons[i].PrevAnalogKeyValue <= 250) {
      if (!i) RearTiresPress ++; ShowPressuresFlag=0; 
      if (i) ProcessPitCombo();
      delay(ShortPressTime);
    }
    if (!i) TimeTmp = millis();
  }
  
  if (buttons[i].AnalogKeyValue>150 && buttons[i].PrevAnalogKeyValue<150) {
    buttons[i].TTmpKeys = millis();
  }
  
  if (buttons[i].AnalogKeyValue != buttons[i].PrevAnalogKeyValue) buttons[i].PrevAnalogKeyValue = buttons[i].AnalogKeyValue;
  }
}



void CheckChangeEncoder(void) {
  unsigned char result = r3.process();
  if (result) {
    Keyboard.write(result == DIR_CCW ? KEY_LEFT_ARROW : KEY_RIGHT_ARROW ); 
  }

}


void CheckFuelEncoder(void) {
  unsigned char result = r1.process();
  if (result) {
    if (result == DIR_CCW && encoderPos < 999) encoderPos++ ;
    if (result == DIR_CW && encoderPos > 0) encoderPos--;   
    ShowNumber(encoderPos,0);
  }
}

void CheckControlEncoder(void) {
  unsigned char result = r2.process();
  if (result) {
    Keyboard.write(result == DIR_CCW ? KEY_UP_ARROW : KEY_DOWN_ARROW);   
  }
}

// Shows 3 digit number beginning from _pos position
void ShowNumber (int _encoderPos, byte _pos) {
  int tmp;
  
  tmp = abs(_encoderPos);
  
//  if (_encoderPos != 0) {
    lc.setDigit(0,_pos,tmp%10,false);
//  } else lc.setRow(0,_pos,0);
  
  tmp /= 10;
  
  if (tmp != 0) {   
   lc.setDigit(0,_pos+1,tmp%10,false);
   tmp /= 10;   //_encoderPos/100;
   if (tmp != 0) {
     lc.setDigit(0,_pos+2,tmp%10,false);
     if (_encoderPos < 0) lc.setRow(0,_pos+3,WDS);
      } else {
        lc.setRow(0,_pos+2,0);
        if (_encoderPos < 0) lc.setRow(0,_pos+2,WDS);
      }
  } else {
    lc.setRow(0,_pos+1,0);
    if (_encoderPos < 0) lc.setRow(0,_pos+1,WDS);
  }
}


void ProcessSwitches(void) {
 for (byte i=0;i<NumSwitches;i++) {
  switch (i) {
      case 0: 
        if (Switches[i].State) PitCombo |= FT; else PitCombo &= ~FT; // switch 0 - request to change front tyres
        break;
      case 1:
        if (Switches[i].State) PitCombo |= RT; else PitCombo &= ~RT; // switch 1 - request to change rear tyres
        break;
      case 2:
        if (Switches[i].State) PitCombo |= WDS; else PitCombo &= ~WDS; // switch 2 - request to clean wildscrean
        break;
      case 3:
        if (Switches[i].State) PitCombo |= FRP; else PitCombo &= ~FRP; // switch 3 - request to make fast repair
    }
 }
 
   if (PitCombo != PrevPitCombo) {
     lc.setRow(0,3,PitCombo);
     PrevPitCombo = PitCombo;
   }
  
}

void CheckSwitches(void) {
 long CheckTime;
 
 for (byte i=0;i<NumSwitches;i++) {
  if (digitalRead(Switches[i].PIN) == LOW) Switches[i].State = true;
   else {
     Switches[i].State = false;
   }
  }
}




