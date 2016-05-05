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
#include "joystick.h"

nrf24l01p wireless; 
symaxProtocol protocol;

unsigned long time = 0;

rx_values_t rxValues;

bool bind_in_progress = false;
unsigned long newTime;

void setup() {  
  // SS pin must be set as output to set SPI to master !
  pinMode(SS, OUTPUT);

  Joystick.begin(false);

  // Set CE pin to 10 and CS pin to 9
  wireless.setPins(10,9);

  // Set power (PWRLOW,PWRMEDIUM,PWRHIGH,PWRMAX)
  wireless.setPwr(PWRLOW);
  
  protocol.init(&wireless);
  
  time = micros();
}

void loop() {
  // put your main code here, to run repeatedly:
  time = micros();
  uint8_t value = protocol.run(&rxValues); 
  newTime = micros();
   
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
      Joystick.setThrottle(rxValues.throttle);
      Joystick.setZAxis(rxValues.yaw);
      Joystick.setYAxis(rxValues.pitch);
      Joystick.setXAxis(rxValues.roll);

      // Flip button
      Joystick.setButton(0, rxValues.flip);

      // Photo button
      //Joystick.setButton(1, rxValues.picture);
      // Video button
      //Joystick.setButton(2, rxValues.flip);
      
      Joystick.sendState();
    break;
    
    case BOUND_NO_VALUES:
    break;
    
    default:
    break;
  }
  delay(2);
}
