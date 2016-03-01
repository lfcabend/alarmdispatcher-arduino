

void toggleDisplay(struct ClockState* s) {
  Serial.println("Display toggle button pressed");
  s->displayOn = !s->displayOn;
  s->backLightStateChanged = true;
  s->someStateChanged = true;
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

boolean buttonPressed(struct Button* button) {
  return button->buttonPressed;
}

void resetButtonPressed(struct Button* button) {
  button->buttonPressed = false;
}

void initializeButton(struct Button* button, int pinNumber) {
  button->buttonPin = pinNumber;
  button->lastButtonState = 0;
  button->buttonState = 0;
  button->lastDebounceTime = 0;
  button->buttonPressed = false;
  pinMode(pinNumber, INPUT);
}

void processSnoozeButton(struct ClockState* s) {
  Serial.println("Snooze button pressed");
  s->ringing = false;
  s->snoozed = true;
  s->someStateChanged = true;
}

void processHotButton(struct ClockState* s) {
  Serial.println("Hotness button pressed");
  s->hot = !s->hot;
  s->ringing = false;
  s->someStateChanged = true;
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

