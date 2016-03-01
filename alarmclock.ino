
#include <SoftwareSerial.h>
#include <Wire.h>
#include "ds3231.h"
//#include "rtc_ds3231.h"
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include "Buttons.h"
#include "CommandProcessor.h"

#define I2C_ADDR 0x27 
#define Rs_pin 0
#define Rw_pin 1
#define En_pin 2
#define BACKLIGHT_PIN 3
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7

#define SNOOZE_INS 'Z'
#define SET_TIME_INS 'T'
#define TURN_OFF_INS 'O'
#define TURN_ON_INS 'F'
#define ACTIVATE_INS 'A'
#define DISPLAY_INS 'D'
#define UPDATE_INS 'U'

#define LCD_UPDATE_INTERVAL 1000
#define CONNECTION_INTERVAL 60000
#define DEBOUNCE_DELAY 50 

#define BACKLIGHT_BUTTON_PIN 2
#define HOT_BUTTON_PIN 3
#define SNOOZE_BUTTON_PIN 4

#define RING_SAFETY 60000
#define ALARM_PIN 7

SoftwareSerial mySerial(10, 11); // RX, TX

LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

struct ClockState {
  boolean snoozed = false;
  boolean hot = false;
  boolean ringing = false;
  boolean displayOn = true;
  boolean backLightStateChanged = false;
  unsigned long timeLCDUpdated = 0;
  unsigned long timeReveivedMessage = 0;   
  boolean someStateChanged = false;
  unsigned long timeStartRinging = 0;
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
 
  lcd.home(); 

  initializeButton(&backlightButton, BACKLIGHT_BUTTON_PIN);
  initializeButton(&hotButton, HOT_BUTTON_PIN);
  initializeButton(&snoozeButton, SNOOZE_BUTTON_PIN);

  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);
}

void loop() {
  // process input
  readLine(&mySerial, &c);

  //if message read then process and update state
  if (c.readMessage) {
    processCommand(&c, &state);
    resetCommand(&c);
    state.timeReveivedMessage = millis();
  }
  processBacklightButton(&backlightButton, &state);
  processHotButton(&hotButton, &state);
  processSnoozeButton(&snoozeButton, &state);

  if (state.displayOn) {
    updateLCD(&state);
  }
  updateBackLight(&state);
  if (state.someStateChanged) {
    sendUpdatedData(&mySerial, &state);
    state.someStateChanged = false;
  }
  if (shouldTurnOff(&state)) {
    state.ringing = false;
  }
  ringTheAlarm(&state);
}

boolean isConnected(struct ClockState* s) {
  unsigned long now = millis();
  return (now - s->timeReveivedMessage) < CONNECTION_INTERVAL;
}

boolean shouldTurnOff(struct ClockState* s) {
  unsigned long now = millis();
  return (now - s->timeStartRinging) > RING_SAFETY;
}

void sendUpdatedData(SoftwareSerial* bluetooth, struct ClockState* s) {  
  char buff[BUFF_MAX];    
  int ringing = convertBooleanToInt(s->ringing);
  int snoozed = convertBooleanToInt(s->snoozed);
  int hot = convertBooleanToInt(s->hot);
  int displayOn = convertBooleanToInt(s->displayOn);
  
  snprintf(buff, BUFF_MAX, "%c%d%d%d%d", UPDATE_INS, 
      ringing, snoozed, hot, displayOn);
  bluetooth->println(buff);
}

int convertBooleanToInt(boolean value) {
  return value ? 1 : 0;
}

void ringTheAlarm(struct ClockState* s) {
  if (s->ringing && s->hot) {
    Serial.println("Ringing alarm");
    digitalWrite(ALARM_PIN, HIGH);
  } else {
    Serial.println("Switching off alarm");
    digitalWrite(ALARM_PIN, LOW);
  }
}




