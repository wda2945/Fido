//
//  PPSconfig.h
//  PPSconfig
//
//  Created by Martin Lane-Smith on 4/22/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//
// PIC32MX170B

#ifndef PPS_CONFIG_h
#define PPS_CONFIG_h

#define GROUP_COUNT 4
#define GROUP_PINS 8
#define MAX_PPS_SIGNALS 8

#define PIN_COUNT 20
#define PIN_NAMES {\
    "RPA0","RPB3","RPB4","RPB7","RPB15",\
    "RPA1","RPB8","RPB1","RPB11","RPB5",\
    "RPA2","RPA4","RPB13","RPB2","PRPB6",\
    "RPA3","RPB0","RPB10","RPB14","RPB9"\
    }

#define PIN_PPS_GROUPS {\
    1,1,1,1,1,\
    2,2,2,2,2,\
    3,3,3,3,3,\
    4,4,4,4,4\
    }

#define ANALOG_PIN_COUNT 12
#define ANALOG_PINS {"RPA0","RPA1","RPB0","RPB1","RPB2","RPB3",0,0,0,"RPB15","RPB14","RPB13"}

#define PACKAGE_PINS {\
    "MCLR","RPA0","RPA1","RPB0","RPB1","RPB2","RPB3","VSS","RPA2","RPA3","RPB4","RPA4","VDD","RPB5",\
    "RPB6","RPB7","RPB8","RPB9","VSS","VCAP","RPB10","RPB11","RB12","RPB13","RPB14","RPB15","AVSS","AVDD"\
    }

#define IN_SIGNAL_GROUPS {\
    {"INT4","SS1","T2CK","IC4","REFCLKI",0,0,0},\
    {"INT3","SDI1","T3CK","IC3","U1CTS","U2RX",0,0},\
    {"INT2","IC1","IC5","U1RX","T4CK","SDI2","OCFB","U2CTS"},\
    {"INT1","IC2","SS2","T5CK","OCFA",0,0,0}\
}
#define OUT_SIGNAL_GROUPS {\
    {"U1TX","OC1","U2RTS","SS1","C2OUT",0,0,0},\
    {"SDO1","SDO2","OC2","C3OUT",0,0,0,0},\
    {"SDO1","OC4","OC5","REFCLKO","SDO2",0,0,0},\
    {"U1RTS","SS2","OC3","U2TX","C1OUT",0,0,0}\
}

#define FIXED_COUNT 13
#define FIXED_SIGNALS 		{"SCK2","SCL1","SCL2","SDA1","SDA2","SCK1","XTAL1","XTAL2"}
#define FIXED_SINGNAL_PINS 	{"RPB15","RPB8","RPB3","RPB9","RPB2","RPB14","RPA2","RPA3"}

#define UART_COUNT 2
#define SPI_COUNT 2
#define I2C_COUNT 2
#define OC_COUNT 5
#define INT_COUNT 4

#endif
