/*
 * File:   Encoder.c
 * Author: martin
 *
 * Created on January 14, 2014
 */
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define _SUPPRESS_PLIB_WARNING
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING

#include "plib.h"

#include "FreeRTOS.h"
#include "task.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSubData.h"

#include "Motor.h"

//mapping list
int EncoderToMotor[4] = ENC_TO_MOTORS;

//quadrature encoder decode
static int encoderLookup[16] = {
//old: 00  01  10  11
        0,  1, -1,  2,    //new state = 00
       -1,  0,  2,  1,    //new state = 01
        1,  2,  0, -1,    //new state = 10
        2, -1,  1,  0};   //new state = 11

int EncoderInit() {
    //configure external interrupts as input
    PORTSetPinsDigitalIn(ENC_1_IOPORT, ENC_1_BIT);
    PORTSetPinsDigitalIn(ENC_2_IOPORT, ENC_2_BIT);
    PORTSetPinsDigitalIn(ENC_3_IOPORT, ENC_3_BIT);
    PORTSetPinsDigitalIn(ENC_4_IOPORT, ENC_4_BIT);

    //outputs from the Encoder mux
    PORTSetPinsDigitalIn(ENCODER_A_IOPORT, ENCODER_A_BIT);
    PORTSetPinsDigitalIn(ENCODER_B_IOPORT, ENCODER_B_BIT);

    //set control pins OD for 5v tolerance
    ENC_0_OPENDRAIN;
    ENC_1_OPENDRAIN;

    //Initialise the external interrupts
    //each INT is an XOR of the two Encoder phases
//INT 1
    IEC0CLR = _IEC0_INT1IE_MASK; //clear int enable
    INTCONCLR = 1 << _INTCON_INT1EP_POSITION;       //falling edge trigger
    //if currently low, set rising trigger
    if (PORTReadBits(INT1_IOPORT, INT1_BIT) == 0) INTCONSET = 1 << _INTCON_INT1EP_POSITION;
    IFS0CLR = _IFS0_INT1IF_MASK;                    //clear flag
    //set priority
    IPC1CLR = _IPC1_INT1IP_MASK;
    IPC1SET = ((ENC_INT_PRIORITY) << _IPC1_INT1IP_POSITION);
    //set sub-priority
    IPC1CLR = _IPC1_INT1IS_MASK;
    IPC1SET = ((ENC_INT_SUB_PRIORITY) << _IPC1_INT1IS_POSITION);
    IEC0SET = _IEC0_INT1IE_MASK;                    //set int enable

//INT 2
    IEC0CLR = _IEC0_INT2IE_MASK;                    //clear int enable
    INTCONCLR = 1 << _INTCON_INT2EP_POSITION;
    if (PORTReadBits(INT2_IOPORT, INT2_BIT) == 0) INTCONSET = 1 << _INTCON_INT2EP_POSITION;
    IFS0CLR = _IFS0_INT2IF_MASK;                    //clear flag
    IPC2CLR = _IPC2_INT2IP_MASK;
    IPC2SET = ((ENC_INT_PRIORITY) << _IPC2_INT2IP_POSITION);
    IPC2CLR = _IPC2_INT2IS_MASK;
    IPC2SET = ((ENC_INT_SUB_PRIORITY) << _IPC2_INT2IS_POSITION);
    IEC0SET = _IEC0_INT2IE_MASK;                    //set int enable

//INT 3
    IEC0CLR = _IEC0_INT3IE_MASK;                    //clear int enable
    INTCONCLR = 1 << _INTCON_INT3EP_POSITION;
    if (PORTReadBits(INT3_IOPORT, INT3_BIT) == 0) INTCONSET = 1 << _INTCON_INT3EP_POSITION;
    IFS0CLR = _IFS0_INT3IF_MASK;                    //clear flag
    IPC3CLR = _IPC3_INT3IP_MASK;
    IPC3SET = ((ENC_INT_PRIORITY) << _IPC3_INT3IP_POSITION);
    IPC3CLR = _IPC3_INT3IS_MASK;
    IPC3SET = ((ENC_INT_SUB_PRIORITY) << _IPC3_INT3IS_POSITION);
    IEC0SET = _IEC0_INT3IE_MASK;                    //set int enable

//INT 4
    IEC0CLR = _IEC0_INT4IE_MASK;                    //clear int enable
    INTCONCLR = 1 << _INTCON_INT4EP_POSITION;
    if (PORTReadBits(INT4_IOPORT, INT4_BIT) == 0) INTCONSET = 1 << _INTCON_INT4EP_POSITION;
    IFS0CLR = _IFS0_INT4IF_MASK;                    //clear flag
    IPC4CLR = _IPC4_INT4IP_MASK;
    IPC4SET = ((ENC_INT_PRIORITY) << _IPC4_INT4IP_POSITION);
    IPC4CLR = _IPC4_INT4IS_MASK;
    IPC4SET = ((ENC_INT_SUB_PRIORITY) << _IPC4_INT4IS_POSITION);
    IEC0SET = _IEC0_INT4IE_MASK;                    //set int enable

    return 0;
}
//set interrupt vectors
extern void __attribute__((interrupt(ENC_IPL), vector(ENC_1_VECTOR))) Encoder_1_ISR_Handler(void);
extern void __attribute__((interrupt(ENC_IPL), vector(ENC_2_VECTOR))) Encoder_2_ISR_Handler(void);
extern void __attribute__((interrupt(ENC_IPL), vector(ENC_3_VECTOR))) Encoder_3_ISR_Handler(void);
extern void __attribute__((interrupt(ENC_IPL), vector(ENC_4_VECTOR))) Encoder_4_ISR_Handler(void);

//common handler
void Encoder_ISR_Handler(int encoder_connector) {
    //called for every transition of every encoder

    //find the relevant motor structure
    int motor = EncoderToMotor[encoder_connector];
    MotorStruct_t *thisMotor = &motors[motor];

    //read the two encoder channels
    unsigned int Achan = PORTReadBits(ENCODER_A_IOPORT, ENCODER_A_BIT);
    unsigned int Bchan = PORTReadBits(ENCODER_B_IOPORT, ENCODER_B_BIT);

    //create a lookup value to determine what the transition means
    unsigned int newState = (Achan ? 2 : 0) | (Bchan ? 1 : 0);           //newState is 0 to 4
    unsigned int lookup = (newState << 2) + thisMotor->lastEncoderState; //combo is 0-15
    int change = encoderLookup[lookup];

    if (change != 2) {              //2 is dummy - bad data. Maybe missed transition?
        thisMotor->encoderCount -= change;
    } else {
        thisMotor->encoderMiss++;   //transition miss count
    }
    thisMotor->lastEncoderState = newState;
}

//Encoder INT handlers
void Encoder_1_ISR_Handler() {
    SET_ENC_MUX_1; //set the mux to read this encoder
    Encoder_ISR_Handler(0);
    ENC_1_CLEAR; //clear flag, flip edge and enable
}

void Encoder_2_ISR_Handler() {
    SET_ENC_MUX_2;
    Encoder_ISR_Handler(1);
    ENC_2_CLEAR; //clear flag, flip edge and enable
}

void Encoder_3_ISR_Handler() {
    SET_ENC_MUX_3;
    Encoder_ISR_Handler(2);
    ENC_3_CLEAR; //clear flag, flip edge and enable
}

void Encoder_4_ISR_Handler() {
    SET_ENC_MUX_4;
    Encoder_ISR_Handler(3);
    ENC_4_CLEAR; //clear flag, flip edge and enable
}