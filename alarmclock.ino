
#include <SoftwareSerial.h>
#include <Wire.h>
#include "ds3231.h"
//#include "rtc_ds3231.h"
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR 0x27 
#define Rs_pin 0
#define Rw_pin 1
#define En_pin 2
#define BACKLIGHT_PIN 3
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7

#define BUFF_MAX 20
#define SNOOZE_INS 'Z'
#define SET_TIME_INS 'T'
#define TURN_OFF_INS 'O'
#define TURN_ON_INS 'F'
#define ACTIVATE_INS 'A'
#define DISPLAY_INS 'D'

#define INTERVAL 1000
#define DEBOUNCE_DELAY 50 

#define BACKLIGHT_BUTTON_PIN 2
#define HOT_BUTTON_PIN 3
#define SNOOZE_BUTTON_PIN 4

SoftwareSerial mySerial(10, 11); // RX, TX

LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);


struct Command {
  char recv[BUFF_MAX];
  unsigned int recv_size = 0;
  boolean readMessage = false;
};

struct ClockState {
  boolean snoozed = false;
  boolean hot = false;
  boolean ringing = false;
  boolean displayOn = true;
  boolean backLightStateChanged = false;
  unsigned long timeLCDUpdated;   
};

struct Button {
  int buttonPin; 
  int buttonState;
  int lastButtonState;
  long lastDebounceTime;
  boolean buttonPressed;
};


struct Command c;
struct ClockState state;
struct Button backlightButton;
struct Button hotButton;
struct Button snoozeButton;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  mySerial.begin(9600);
  Wire.begin();
  DS3231_init(DS3231_INTCN);
  memset(c.recv, 0, BUFF_MAX);
  lcd.begin (16,2); 
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
 
  lcd.home (); 

  initializeButton(&backlightButton, BACKLIGHT_BUTTON_PIN);
  initializeButton(&hotButton, HOT_BUTTON_PIN);
  initializeButton(&snoozeButton, SNOOZE_BUTTON_PIN);

}

void loop() {
  // process input
  readLine(&mySerial, &c);

  //if message read then process and update state
  if (c.readMessage) {
    processCommand(&c, &state);
    resetCommand(&c);
  }
  processBacklightButton(&backlightButton, &state);
//  processHotButton(&hotButton, &state);
//  processSnoozeButton(&snoozeButton, &state);

  if (state.displayOn) {
    updateLCD(&state);
  }
  updateBackLight(&state);
}

void updateBackLight(struct ClockState* s) {
  if (s->backLightStateChanged) {
    lcd.setBacklight(s->displayOn ? HIGH : LOW);
    s->backLightStateChanged = false;
  }
}

void updateLCD(struct ClockState* s) {
  unsigned long now = millis();
  struct ts t;
  char buff[BUFF_MAX];    

  if ((now - s->timeLCDUpdated > INTERVAL)) {
      DS3231_get(&t);
      snprintf(buff, BUFF_MAX, "%02d:%02d:%02d", t.hour, t.min, t.sec);
      lcd.setCursor(8,0);
      lcd.print(buff);
      s->timeLCDUpdated = now;
    
      snprintf(buff, BUFF_MAX, "%02d/%02d/%02d", t.mday,
           t.mon, t.year);
      lcd.setCursor (6,1);
      lcd.print(buff);    

      lcd.setCursor (5,0);
      switch(t.wday) {
        case 7: lcd.print("ZA");
          break; 
        case 6: lcd.print("VR");
          break; 
        case 5: lcd.print("DO");
          break; 
        case 4: lcd.print("WO");
          break; 
        case 3: lcd.print("DI");
          break; 
        case 2: lcd.print("MA");
          break;
        case 1: lcd.print("ZO");
          break;              
      }
  }
}

void processCommand(struct Command* com, struct ClockState* s) {
  char ins = getInstructionFromCommand(com);
  switch (ins) {
    case SNOOZE_INS: 
      snooze(com, s);
      break;
   case SET_TIME_INS: 
      setTime(com, s);
      break;   
   case TURN_OFF_INS: 
      turnOff(com, s);
      break;   
   case TURN_ON_INS: 
      turnOn(com, s);
      break;
   case ACTIVATE_INS: 
      activate(com, s);
      break;
   case DISPLAY_INS: 
      updateDispay(com, s);
      break;            
  }
}

void toggleDisplay(struct ClockState* s) {
  Serial.println("Display toggle button pressed");
  s->displayOn = !s->displayOn;
  s->backLightStateChanged = true;
}

void buttonActionHander(struct Button* button, 
    struct ClockState* s, void (*f)(struct ClockState*) ) {
  updateButtonState(button);
  if (buttonPressed(button)) {
    f(s);
    resetButtonPressed(button);
  }
}

void processBacklightButton(struct Button* button, struct ClockState* s) {
  buttonActionHander(button, s, toggleDisplay);
}

void processHotButton(struct Button* button, struct ClockState* s) {
  buttonActionHander(button, s, processHotButton);
}

void processSnoozeButton(struct Button* button, struct ClockState* s) {
  buttonActionHander(button, s, processSnoozeButton);
}

char getInstructionFromCommand(struct Command* com) {
  return com->recv[0];
}

void resetCommand(struct Command* com) {
    com->recv_size = 0;
    com->recv[0] = 0;
    com->readMessage = false;
}

void processSnoozeButton(struct ClockState* s) {
  Serial.println("Snooze button pressed");
  s->ringing = false;
  s->snoozed = true;
}

void processHotButton(struct ClockState* s) {
  Serial.println("Hotness button pressed");
  s->hot = !s->hot;
  s->ringing = false;
}

void updateDispay(struct Command* com, struct ClockState* s) {
  lcd.setBacklight(HIGH);
  Serial.println("Update display");        
  s->backLightStateChanged = true;
  char activate = com->recv[1];
  if (activate == '0') {
    Serial.println("Switchin off display");      
    s->displayOn = false;
  } else if (activate == '1') {
    Serial.println("Switchin on display");    
    s->displayOn = true;
  }
}

void snooze(struct Command* com, struct ClockState* s) {
  Serial.println("Snoozing");      
  s->snoozed = true;
}

void setTime(struct Command* com, struct ClockState* s) {
  Serial.println("Setting time");      
  Serial.println(com->recv);      
  setTimeInClock(com->recv);
}

void turnOff(struct Command* com, struct ClockState* s) {
  Serial.println("Turing off");      
  s->ringing = false;
}

void turnOn(struct Command* com, struct ClockState* s) {
  Serial.println("Turing on");      
  s->ringing = true;
}

void activate(struct Command* com, struct ClockState* s) {
  Serial.println("Switch activation");      
  char activate = com->recv[1];
  if (activate == '0') {
    Serial.println("Switchin off");      
    s->hot = false;
  } else if (activate == '1') {
    Serial.println("Switchin on");    
    s->hot = false;
  }  
}


void readLine(SoftwareSerial* s, struct Command* com) {
  char in;
  if (s->available() > 0) {
    in = s->read();
    if ((in == 10 || in == 13) && (com->recv_size > 0)) {
        com->readMessage = true;
    } else if (in < 48 || in > 122) {;       // ignore ~[0-9A-Za-z]
    } else if (com->recv_size > BUFF_MAX - 2) {   // drop lines that are too long
        com->recv_size = 0;
        com->recv[0] = 0;
        com->readMessage = false;
    } else if (com->recv_size < BUFF_MAX - 2) {
        com->recv[com->recv_size] = in;
        com->recv[com->recv_size + 1] = 0;
        com->recv_size += 1;
    }
  }
}

void setTimeInClock(char *cmd) {
    struct ts t;

    //T355720619112011
    t.sec = inp2toi(cmd, 1);
    t.min = inp2toi(cmd, 3);
    t.hour = inp2toi(cmd, 5);
    t.wday = cmd[7] - 48;
    t.mday = inp2toi(cmd, 8);
    t.mon = inp2toi(cmd, 10);
    t.year = inp2toi(cmd, 12) * 100 + inp2toi(cmd, 14);
    DS3231_set(t);
}

void initializeButton(struct Button* button, int pinNumber) {
  button->buttonPin = pinNumber;
  button->lastButtonState = 0;
  button->buttonState = 0;
  button->lastDebounceTime = 0;
  button->buttonPressed = false;
  pinMode(pinNumber, INPUT);
}

void updateButtonState(struct Button* button) {
   int reading = digitalRead(button->buttonPin);
   
  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != button->lastButtonState) {
    // reset the debouncing timer
    button->lastDebounceTime = millis();
  }

  if ((millis() - button->lastDebounceTime) > DEBOUNCE_DELAY) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:
    
    // if the button state has changed:
    if (reading != button->buttonState) {
   
      button->buttonState = reading;

      // only toggle the state changed if the new button state is HIGH
      if (button->buttonState == HIGH) {
        Serial.println("Button pressed");

        button->buttonPressed = true;
      }
    }
  }
  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  button->lastButtonState = reading;

}

boolean buttonPressed(struct Button* button) {
  return button->buttonPressed;
}

void resetButtonPressed(struct Button* button) {
  button->buttonPressed = false;
}

