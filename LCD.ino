

void updateLCD(struct ClockState* s) {
  unsigned long now = millis();
  struct ts t;

  if ((now - s->timeLCDUpdated > LCD_UPDATE_INTERVAL)) {
      DS3231_get(&t);      
      updateTimeOnLcd(&t);
      updateDateOnLcd(&t);
      updateWeekDayOnLcd(&t);
      updateHotnessOnLcd(s);
      updateConnectedOnLcd(s);
      updateRingingOnLcd(s);
      s->timeLCDUpdated = now;
  }
}

void updateDispay(struct Command* com, struct ClockState* s) {
  lcd.setBacklight(HIGH);
  s->someStateChanged = true;
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


void updateBackLight(struct ClockState* s) {
  if (s->backLightStateChanged) {
    lcd.setBacklight(s->displayOn ? HIGH : LOW);
    s->backLightStateChanged = false;
  }
}

void updateHotnessOnLcd(struct ClockState* s) {
  lcd.setCursor(0, 0);
  if (s->hot) {
    lcd.print("On ");
  } else {
    lcd.print("Off");
  }
}

void updateRingingOnLcd(struct ClockState* s) {
  lcd.setCursor(2, 1);
  if (s->ringing) {
    lcd.print("*");
  } else {
    lcd.print("-");
  }
}

void updateConnectedOnLcd(struct ClockState* s) {
  lcd.setCursor(0, 1);
  if (isConnected(s)) {
    lcd.print("*");
  } else {
    lcd.print("-");
  }
}

void updateTimeOnLcd(struct ts* t) {
  char buff[BUFF_MAX];    
  snprintf(buff, BUFF_MAX, "%02d:%02d:%02d", t->hour, t->min, t->sec);
  lcd.setCursor(8,0);
  lcd.print(buff);
}

void updateDateOnLcd(struct ts* t) {
  char buff[BUFF_MAX];    
  snprintf(buff, BUFF_MAX, "%02d/%02d/%02d", t->mday,
           t->mon, t->year);
  lcd.setCursor (6,1);
  lcd.print(buff);    
}

void updateWeekDayOnLcd(struct ts* t) {
  lcd.setCursor (5,0);
  switch(t->wday) {
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



