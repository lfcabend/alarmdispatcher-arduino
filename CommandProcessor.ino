
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

void snooze(struct Command* com, struct ClockState* s) {
  Serial.println("Snoozing");      
  s->snoozed = true;
  s->someStateChanged = true;
}

void setTime(struct Command* com, struct ClockState* s) {
  Serial.println("Setting time");      
  Serial.println(com->recv);      
  setTimeInClock(com->recv);
  s->someStateChanged = true;
}

void turnOff(struct Command* com, struct ClockState* s) {
  Serial.println("Turing off");      
  s->ringing = false;
  s->snoozed = false;
  s->someStateChanged = true;
}

void turnOn(struct Command* com, struct ClockState* s) {
  Serial.println("Turing on");      
  unsigned long now = millis();
  
  s->ringing = true;
  s->snoozed = false;
  s->timeStartRinging = now;
  s->someStateChanged = true;
}

void activate(struct Command* com, struct ClockState* s) {
  Serial.println("Switch activation");      
  char activate = com->recv[1];
  s->someStateChanged = true;
  if (activate == '0') {
    Serial.println("Switchin off");      
    s->hot = false;
  } else if (activate == '1') {
    Serial.println("Switchin on");    
    s->hot = true;
  }  
}

void resetCommand(struct Command* com) {
    com->recv_size = 0;
    com->recv[0] = 0;
    com->readMessage = false;
}

char getInstructionFromCommand(struct Command* com) {
  return com->recv[0];
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

