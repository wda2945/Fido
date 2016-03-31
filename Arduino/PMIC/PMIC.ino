

//PMIC code for FIDO using ATTiny85
//
//Jan 24 2015
//
//  I2C slave to MCP
//  Power enable PMIC & MCP
//  Motor Inhibit
//  User switch

#include <TinyWireS.h>
#include <usiTwiSlave.h>
#include "PMIC_msg.h"

/*
Reset          o 1   8 o Vcc
Arduino Pin 3  o 2   7 o Arduino Pin 2 (+SCL)
Arduino Pin 4  o 3   6 o Arduino Pin 1
        Gnd    o 4   5 o Arduino Pin 0 (+SDA)
*/

#define SW_PIN           4              // ATtiny Pin 4
#define PWR_PIN          3              // Pin 2
#define MOT_PIN          1              // Pin 6
//I2C on Arduino Pins 0 & 2 (ATTiny Pins 5 & 7)

#define DEBOUNCE    100    //ON duration to ignore switch bounces (mS)
#define LONG_PRESS  2000   //ON duration to power off (mS)

PMICstate_enum PMICstate;

unsigned long buttonDownTime = 0;  //timing button presses
byte lastButton = false;           //state when last around the loop

void setup(){ 
  pinMode(SW_PIN,INPUT);     
  pinMode(PWR_PIN,OUTPUT);     
  pinMode(MOT_PIN,OUTPUT);     

  digitalWrite(PWR_PIN,LOW);   //MCP off for now
  digitalWrite(MOT_PIN,HIGH);  //Motors reset

  PMICstate = PMIC_POWER_OFF;

  TinyWireS.begin(PMIC_I2C_ADDRESS);      // init I2C Slave mode
}

void loop(){
  int buttonPressDuration = 0;
  byte byteRcvd = 0;
  byte buttonPressed = digitalRead(SW_PIN);  //pressed is truw, unpressed is false

  if (buttonPressed)
  {
    //button pressed
    if (!lastButton)
    {
      //new press - start timing
      buttonDownTime = millis();
    }
    else
    {
      //current duration
      buttonPressDuration = millis() - buttonDownTime;
    }
  }
  else
  {
    //button up
    if (lastButton)
    {
      //just released
      buttonPressDuration = millis() - buttonDownTime;
    }
  }
  lastButton = buttonPressed;

  if (TinyWireS.available()){           // got I2C input
    byteRcvd = TinyWireS.receive();     // get the byte from MCP

    switch(byteRcvd)
    {
    case PMIC_REPORT_STATE:
      TinyWireS.send(PMICstate & 0xf);
      break;
    case PMIC_SET_ACTIVE:   
      if (PMICstate == PMIC_STANDBY)
      {
        PMICstate = PMIC_ACTIVE;
        digitalWrite(MOT_PIN,LOW);  //enable motors
      }
      break;
    case PMIC_SET_STANDBY:  
      if (PMICstate == PMIC_ACTIVE)
      {
        PMICstate = PMIC_STANDBY;
        digitalWrite(MOT_PIN,HIGH);  //disable motors
      }
      break;
    case PMIC_SET_OFF:
      PMICstate = PMIC_POWERING_OFF;
      delay(200);
      digitalWrite(PWR_PIN,LOW);  //we're outa here
      break;
    default:
      break;    
    }
  }

  switch (PMICstate)
  {
  case PMIC_POWER_OFF:
    //just started
    if (buttonPressed && (buttonPressDuration > DEBOUNCE))
    {
      //button held down beyond key bounce time
      digitalWrite(PWR_PIN,HIGH);  //power will now stay on when button released
    }
    if (buttonPressed && (buttonPressDuration > LONG_PRESS))
    {
      PMICstate = PMIC_POWERING_OFF;
      digitalWrite(PWR_PIN,LOW);  //power will now drop when button released
    }    
    if (!buttonPressed)
    {
      //button up, still running
      PMICstate = PMIC_STANDBY;
    }
    break;
  case PMIC_STANDBY:
     if (!buttonPressed && (buttonPressDuration > DEBOUNCE))
    {
      PMICstate = PMIC_ACTIVE;
      digitalWrite(MOT_PIN,LOW);  //switch to active
    }
    if (buttonPressed && (buttonPressDuration > LONG_PRESS))
    {
      PMICstate = PMIC_POWERING_OFF;
      digitalWrite(MOT_PIN,HIGH);  //disable motors
      digitalWrite(PWR_PIN,LOW);  //power will now drop when button released
    }
    break;
  case PMIC_ACTIVE:
    if (!buttonPressed && (buttonPressDuration > DEBOUNCE))
    {
      PMICstate = PMIC_STANDBY;
      digitalWrite(MOT_PIN,HIGH);  //switch to standby
    }
    if (buttonPressed && (buttonPressDuration > LONG_PRESS))
    {
      PMICstate = PMIC_POWERING_OFF;
      digitalWrite(MOT_PIN,HIGH);     
      digitalWrite(PWR_PIN,LOW);  //power will now drop when button released
    }
    break;
  case PMIC_POWERING_OFF:
    break;
  }
}


