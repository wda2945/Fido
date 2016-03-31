//
//  ChipKitDP32PPS.h
//  PPSconfig
//
//  Created by Martin Lane-Smith on 4/22/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#ifndef PPSconfig_ChipKitDP32PPS_h
#define PPSconfig_ChipKitDP32PPS_h

#define GROUP_COUNT 4
#define GROUP_PINS 8
#define MAX_PPS_SIGNALS 8

#define PIN_COUNT 18
char *pinName[PIN_COUNT] = {
    "RPB3","RPB4","RPB7",
    "RPA1","RPB8","RPB1","RPB11","RPB5",
    "RPA2","RPA4","RPB13","RPB2",
    "RPB0","RPB10","RPB14","RPB9"};

char PPSGroup[PIN_COUNT]  = {
    1,1,1,
    2,2,2,2,2,
    3,3,3,3,
    4,4,4,4,};

#define AN_COUNT 12
char *analogPins[] = {"RPA0","RPA1","RPB0","RPB1","RPB2","RPB3",0,0,0,0,"RPB14","RPB13",};

char *connectorPins[PIN_COUNT] = {
    "RPA0","RPA1","RPB0","RPB1","RPB2","RPB3","RPA2","RPA3","RPB4",
    "RPA4","RPB5","RPB7","RPB8","RPB9","RPB10","RPB11","RPB13","RPB14"};
char connectorNumber[PIN_COUNT] = {2,3,4,5,6,7,9,10,11,12,14,16,17,18,21,22,24,25};

char *PPSinSignals[GROUP_COUNT][MAX_PPS_SIGNALS] = {
    {"INT4","SS1","T2CK","IC4","REFCLKI",0,0,0},
    {"INT3","SDI1","T3CK","IC3","U1CTS","U2RX",0,0},
    {"INT2","IC1","IC5","U1RX","T4CK","SDI2","OCFB","U2CTS"},
    {"INT1","IC2","SS2","T5CK","OCFA",0,0,0}
};
char *PPSoutSignals[GROUP_COUNT][MAX_PPS_SIGNALS] = {
    {"U1TX","OC1","U2RTS","SS1","C2OUT",0,0,0},
    {"SDO1","SDO2","OC2","C3OUT",0,0,0,0},
    {"SDO1","OC4","OC5","REFCLKO","SDO2",0,0,0},
    {"U1RTS","SS2","OC3","U2TX","C1OUT",0,0,0}
};

#define FIXED_COUNT 13
char *fixedSignals[] = {"PRG","SCK2","SCL1","SCL2","SDA1","SDA2","USBD-","USBID","USBD+","VBusOn","SCK1","XTAL1","XTAL2"};
char *fixedPins[] = {"RPA8","RPB15","RPB8","RPB3","RPB9","RPB2","RPB11","RPB5","RPB10","RPB14","RPB14","RPA2","RPA3"};

#define UART_COUNT 2
#define SPI_COUNT 2
#define I2C_COUNT 2
#define OC_COUNT 5
#define INT_COUNT 4


#endif
