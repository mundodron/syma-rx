/**
 * Syma controller for Fly Simulator using Arduino Leonardo.
 * 
 * Arduino Leonardo or Arduino Micro (or any other ATmega32u4 microcontroller)
 * NRF24L01
 * 
 * You may also use Arduino UNO with UnoJoy.
 *
 * 
 * Copyright (C) 2016 Juan Pedro Gonzalez Gutierrez
 */
#include <SPI.h>
#include "symax_protocol.h"
#include <Joystick.h>

#define SERIAL_DEBUG true

nrf24l01p wireless; 
symaxProtocol protocol;

unsigned long time = 0;

rx_values_t rxValues;

// Status led LED settings
#define LED_PIN 7
unsigned long statusLedChangeTime = 0;
byte statusLedState = LOW;

#if SERIAL_DEBUG
  uint8_t lastState = 255;
#endif

// Create Joystick
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_JOYSTICK, 4, 0,
  true, true, false, false, false, false,
  true, true, false, false, false);

bool bind_in_progress = false;
unsigned long newTime;

void setup() {
#if SERIAL_DEBUG
  Serial1.begin(115200);
  while (!Serial1) 
  {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
#endif
  // SS pin must be set as output to set SPI to master !
  pinMode(SS, OUTPUT);

  // Set status LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Set Range Values
  Joystick.setXAxisRange(127, -127);
  Joystick.setYAxisRange(127, -127);
  //Joystick.setZAxisRange(127, -127);
  Joystick.setRudderRange(127, -127);
  Joystick.setThrottleRange(0, 255);
  
  Joystick.begin(false);
  
  // Set CE pin to 10 and CS pin to 9
  wireless.setPins(10,9);

  // Set power (PWRLOW,PWRMEDIUM,PWRHIGH,PWRMAX)
  wireless.setPwr(PWRLOW);
  
  protocol.init(&wireless);

#if SERIAL_DEBUG
  Serial1.println("Ready...");
#endif

  time = micros();
}

void loop() {
  // put your main code here, to run repeatedly:
  time = micros();
  unsigned long currentMillis = millis();
  
  uint8_t value = protocol.run(&rxValues); 
  newTime = micros();

  // Status LED
  if (protocol.getState() == BOUND) {
    digitalWrite(LED_PIN, HIGH);
    statusLedState = HIGH;
  } else {
    // Blink the LED  
    if ( currentMillis - statusLedChangeTime > 200) {
      if (statusLedState == LOW) {
        digitalWrite(LED_PIN, HIGH);
        statusLedState = HIGH;
      }
      else 
      {
        digitalWrite(LED_PIN, LOW);
        statusLedState = LOW;
      }
      statusLedChangeTime = millis();
    }
  }

#if SERIAL_DEBUG
  uint8_t currentState = protocol.getState();
  if (currentState != lastState) 
  {
    switch(currentState) 
    {
      case BOUND:
        Serial1.println(F("Bound"));
        break;
      case NO_BIND:
        Serial1.println(F("Not bound"));
      case WAIT_FIRST_SYNCHRO:
        Serial1.println(F("Waiting first synchronization."));
        break;
    }
    lastState = currentState;
  }
#endif
  
  switch( value )
  {
    case  BIND_IN_PROGRESS:
      if(!bind_in_progress)
      {
        bind_in_progress = true;
      }
    break;
    
    case BOUND_NEW_VALUES:
      // Sticks
      #if SERIAL_DEBUG
      Serial1.print(F("Throttle: "));
      Serial1.print(rxValues.throttle);
      Serial1.print(F(" Yaw: "));
      Serial1.print(rxValues.yaw);
      Serial1.print(F(" Pitch: "));
      Serial1.print(rxValues.pitch);
      Serial1.print(F(" Roll: "));
      Serial1.print(rxValues.roll);
      Serial1.println();
      #endif
      
      Joystick.setThrottle((int16_t)rxValues.throttle);
      //Joystick.setZAxis((int16_t)rxValues.yaw);
      Joystick.setRudder((int16_t)rxValues.yaw);
      Joystick.setYAxis((int16_t)rxValues.pitch);
      Joystick.setXAxis((int16_t)rxValues.roll);

      // Flip button
      Joystick.setButton(0, rxValues.flip);

      // Speed change
      Joystick.setButton(1, rxValues.highspeed);
      // Photo button
      Joystick.setButton(2, rxValues.picture);
      // Video button
      Joystick.setButton(3, rxValues.video);
      
      Joystick.sendState();
    break;
    
    case BOUND_NO_VALUES:
    break;
    
    default:
    break;
  }
  //delay(2);
}
