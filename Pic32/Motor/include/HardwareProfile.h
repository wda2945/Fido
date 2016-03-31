/***********************************************
 *  Hardware profile for Motor Control
 ***********************************************/

#ifndef _HARDWARE_PROFILE_H
#define _HARDWARE_PROFILE_H

//#include <plib.h>
#include "Fubarino.h"
    
    #define CLOCK_FREQ              80000000ul
    #define GetSystemClock()        (CLOCK_FREQ)
    #define GetPeripheralClock()    (CLOCK_FREQ/(1 << OSCCONbits.PBDIV))
    #define GetInstructionClock()   (CLOCK_FREQ)

//MOT Pin Usage

//index for motor table - matches connector use
typedef enum {FRONT_LEFT_MOTOR, FRONT_RIGHT_MOTOR,
        REAR_RIGHT_MOTOR, REAR_LEFT_MOTOR, NUM_MOTORS} MotorsEnum_t;

//Encoder connectors are numbered clockwise from top-left 1-4
        
//Pin 0 Enc 3 INT1
#define INT1_IOPORT    PIN_0_IOPORT    //INT1
#define INT1_BIT       PIN_0_BIT
#define USING_INT_1
#define USING_PIN_0     1

#define INT2_IOPORT    PIN_1_IOPORT    //INT2
#define INT2_BIT       PIN_1_BIT
#define USING_INT_2
#define USING_PIN_1     1

#define INT3_IOPORT    PIN_2_IOPORT    //INT3
#define INT3_BIT       PIN_2_BIT
#define USING_INT_3
#define USING_PIN_2     1

#define INT4_IOPORT    PIN_3_IOPORT    //INT4
#define INT4_BIT       PIN_3_BIT
#define USING_INT_4
#define USING_PIN_3     1

#define MOT_RST_IOPORT  PIN_4_IOPORT    //Pin 1 Motor Driver reset
#define MOT_RST_BIT     PIN_4_BIT
#define USING_PIN_4     1

#define EMUX0_IOPORT            PIN_5_IOPORT  //Select0 to Dual 4:1 mux
#define EMUX0_BIT               PIN_5_BIT
#define ENC_0_OPENDRAIN         mPORTCOpenDrainOpen(PIN_5_BIT);
#define EMUX1_IOPORT            PIN_6_IOPORT  //Select1 to Dual 4:1 mux
#define EMUX1_BIT               PIN_6_BIT
#define ENC_1_OPENDRAIN         mPORTCOpenDrainOpen(PIN_6_BIT);


//Motor driver connectors are numbered clockwise from top-left 1-4
#define PWM_3_IOPORT    PIN_7_IOPORT    //OC2
#define PWM_3_BIT       PIN_7_BIT
#define USING_PIN_7     1
#define PWM_2_IOPORT    PIN_8_IOPORT    //OC3
#define PWM_2_BIT       PIN_8_BIT
#define USING_PIN_8     1
#define PWM_4_IOPORT    PIN_9_IOPORT    //OC4
#define PWM_4_BIT       PIN_9_BIT
#define USING_PIN_9     1
#define PWM_1_IOPORT    PIN_10_IOPORT   //OC5
#define PWM_1_BIT       PIN_10_BIT
#define USING_PIN_10    1
#define ENCODER_A_IOPORT        PIN_11_IOPORT   //Encoder Mux Phase A
#define ENCODER_A_BIT           PIN_11_BIT
#define USING_PIN_11         1
#define ENCODER_B_IOPORT        PIN_12_IOPORT   //Encoder Mux Phase B
#define ENCODER_B_BIT           PIN_12_BIT
#define USING_PIN_12         1
#define DIR_1_IOPORT    PIN_13_IOPORT
#define DIR_1_BIT       PIN_13_BIT
#define DIR_4_IOPORT    PIN_14_IOPORT
#define DIR_4_BIT       PIN_14_BIT
#define DIR_2_IOPORT    PIN_15_IOPORT
#define DIR_2_BIT       PIN_15_BIT
#define DIR_3_IOPORT    PIN_16_IOPORT
#define DIR_3_BIT       PIN_16_BIT

//define where motor drivers are plugged
#define MOTORS_TO_OC        {5, 3, 2, 4}
#define MOTORS_TO_CONNECTOR {1, 2, 3, 4}
//#define PWM_TO_MOTORS {FRONT_LEFT_MOTOR, FRONT_RIGHT_MOTOR, REAR_RIGHT_MOTOR, REAR_LEFT_MOTOR}

#define FAULT_MUX_IOPORT        PIN_17_IOPORT   //Output of 16:1 mux
#define FAULT_MUX_BIT           PIN_17_BIT
#define USING_PIN_17            1

#define FMUX3_IOPORT            PIN_18_IOPORT   //Select into 16:1 mux
#define FMUX3_BIT               PIN_18_BIT
#define FMUX2_IOPORT            PIN_19_IOPORT   //Select into 16:1 mux
#define FMUX2_BIT               PIN_19_BIT
#define FMUX1_IOPORT            PIN_20_IOPORT   //Select into 16:1 mux
#define FMUX1_BIT               PIN_20_BIT
#define USER_LED_IOPORT         PIN_21_IOPORT   //User LED on board
#define USER_LED_BIT            PIN_21_BIT
#define FMUX0_IOPORT            PIN_22_IOPORT   //Select into 16:1 mux
#define FMUX0_BIT               PIN_22_BIT

enum FAULT_SOURCES {UNUSED_FAULT, MOTOR_FAULT1, MOTOR_FAULT2, CURRENT_FAULT};
#define FMUX_SOURCES {UNUSED_FAULT, UNUSED_FAULT, UNUSED_FAULT, UNUSED_FAULT, \
            MOTOR_FAULT1, MOTOR_FAULT2, MOTOR_FAULT1, MOTOR_FAULT2, MOTOR_FAULT1, MOTOR_FAULT2, \
            CURRENT_FAULT, CURRENT_FAULT, CURRENT_FAULT, \
            MOTOR_FAULT1, MOTOR_FAULT2, CURRENT_FAULT}
#define FMUX_CONNECTORS {0,0,0,0,3,3,4,4,1,1,1,4,2,2,2,2}

//Pin 23

//Pin 24    Log UART Tx
//Pin 25    Rx from MCP
//Pin 26    Tx to MCP
#define SPI_SS_IOPORT           PIN_27_IOPORT   //To hold down SPI SS pin - in case SD Card present
#define SPI_SS_BIT              PIN_27_BIT
//Pin A14
//Pin A13
//Pin A12
//Pin A11
//Pin A10
//Pin A9 ICSP
//Pin A8 ICSP
//Pin A7
#define AMPS_4_IOPORT   PIN_A6_IOPORT
#define AMPS_4_BIT      PIN_A6_BIT
#define USING_PIN_A6    1
#define AMPS_1_IOPORT   PIN_A5_IOPORT
#define AMPS_1_BIT      PIN_A5_BIT
#define USING_PIN_A5    1
#define AMPS_3_IOPORT   PIN_A4_IOPORT
#define AMPS_3_BIT      PIN_A4_BIT
#define USING_PIN_A4    1
#define AMPS_2_IOPORT   PIN_A3_IOPORT
#define AMPS_2_BIT      PIN_A3_BIT
#define USING_PIN_A3    1

#define AMPS_CONFIG_PORT  (ENABLE_AN12_ANA | ENABLE_AN11_ANA | ENABLE_AN10_ANA | ENABLE_AN9_ANA)
#define AMPS_CONFIG_SCAN  (SKIP_SCAN_AN12 | SKIP_SCAN_AN11 | SKIP_SCAN_AN10 | SKIP_SCAN_AN9)

#define AN_TO_CONNECTORS {2,3,1,4}
//define where current sensors are plugged
#define AN_TO_MOTORS {FRONT_LEFT_MOTOR, FRONT_RIGHT_MOTOR, REAR_LEFT_MOTOR, REAR_RIGHT_MOTOR}


//define where encoders are plugged
#define ENC_TO_MOTORS {FRONT_LEFT_MOTOR, FRONT_RIGHT_MOTOR,\
                        REAR_LEFT_MOTOR, REAR_RIGHT_MOTOR}

//encoder interrupt pins
#define ENC_1_IOPORT INT4_IOPORT
#define ENC_1_BIT    INT4_BIT
#define ENC_2_IOPORT INT2_IOPORT
#define ENC_2_BIT    INT2_BIT
#define ENC_3_IOPORT INT3_IOPORT
#define ENC_3_BIT    INT3_BIT
#define ENC_4_IOPORT INT1_IOPORT
#define ENC_4_BIT    INT1_BIT

#define ENC_1_CLEAR     IEC0CLR = _IEC0_INT4IE_MASK;\
    IFS0CLR = _IFS0_INT4IF_MASK;\
    INTCONINV = 1 << _INTCON_INT4EP_POSITION;\
    IEC0SET = _IEC0_INT4IE_MASK;
#define ENC_2_CLEAR     IEC0CLR = _IEC0_INT2IE_MASK;\
    IFS0CLR = _IFS0_INT2IF_MASK;\
    INTCONINV = 1 << _INTCON_INT2EP_POSITION;\
    IEC0SET = _IEC0_INT2IE_MASK;
#define ENC_3_CLEAR     IEC0CLR = _IEC0_INT3IE_MASK;\
    IFS0CLR = _IFS0_INT3IF_MASK;\
    INTCONINV = 1 << _INTCON_INT3EP_POSITION;\
    IEC0SET = _IEC0_INT3IE_MASK;
#define ENC_4_CLEAR     IEC0CLR = _IEC0_INT1IE_MASK;\
    IFS0CLR = _IFS0_INT1IF_MASK;\
    INTCONINV = 1 << _INTCON_INT1EP_POSITION;\
    IEC0SET = _IEC0_INT1IE_MASK;

//Encoder interrupt vectors - for the wrapper
#define ENC_1_VECTOR           _EXTERNAL_4_VECTOR
#define ENC_2_VECTOR           _EXTERNAL_2_VECTOR
#define ENC_3_VECTOR           _EXTERNAL_3_VECTOR
#define ENC_4_VECTOR           _EXTERNAL_1_VECTOR
//Encoder Mux setting
//#define ENC_CONN_EMUX_CHANNELS     {2, 0, 1, 3}

#define SET_ENC_MUX_3      PORTSetBits(EMUX1_IOPORT, EMUX1_BIT);PORTSetBits(EMUX0_IOPORT, EMUX0_BIT);


#define SET_ENC_MUX_2      PORTClearBits(EMUX1_IOPORT, EMUX1_BIT);PORTClearBits(EMUX0_IOPORT, EMUX0_BIT);
#define SET_ENC_MUX_1      PORTClearBits(EMUX1_IOPORT, EMUX1_BIT);PORTSetBits(EMUX0_IOPORT, EMUX0_BIT);
//#define SET_ENC_MUX_4      PORTSetBits(EMUX1_IOPORT, EMUX1_BIT);PORTSetBits(EMUX0_IOPORT, EMUX0_BIT);
#define SET_ENC_MUX_4      PORTSetBits(EMUX1_IOPORT, EMUX1_BIT);PORTClearBits(EMUX0_IOPORT, EMUX0_BIT);

#ifdef	__cplusplus
extern "C" {
#endif

    void BoardInit(void);

#ifdef	__cplusplus
}
#endif

#endif
