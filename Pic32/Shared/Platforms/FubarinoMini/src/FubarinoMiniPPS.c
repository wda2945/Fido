//
//  FubarinoMiniPPS.c
//  PPSconfig
//
//  Created by Martin Lane-Smith on 4/22/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//
#include "FubarinoMiniPPS.h"

char *PPSPinName[PPS_PIN_COUNT] = {
    "RPB15","RPB3","RPB4","RPB7","RPC0","RPC5","RPC7",
    "RPA1","RPA8","RPA9","RPB1","RPB11","RPB5","RPB8","RPC8",
    "RPA2","RPA4","RPB13","RPB2","RPC1","RPC3","RPC6",
    "RPB0","RPB10","RPB14","RPB9","RPC2","RPC4","RPC9"};

char PPSGroup[PPS_PIN_COUNT]  = {
    1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,
    4,4,4,4,4,4,4};

char *analogPins[AN_COUNT] = {"RPA0","RPA1","RPB0","RPB1","RPB2","RPB3","RPC0","RPC1","RPC2","RPB15","RPB14","RPB13","RPC3"};

char *connectorPins[PIN_COUNT] = {"RPB13","RPA10","RPA7","RPB14","RPB15","RPA0","RPA1","RPB0","RPB1","RPB2","RPB3","RPC0",
    "RPC1","RPC2","RPA8","RPB11","RPB10","RPC9","RPC8","RPC7","RPC6","RPB9","RPB8","RPB7","RPB5","RPC5",
    "RPC4","RPC3","RPA9","RPA4","RPB4"};
char connectorNumber[PIN_COUNT] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,16,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17};

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

char *fixedSignals[FIXED_COUNT] = {"PRG","SCK2","SCL1","SCL2","SDA1","SDA2","USBD-","USBID","USBD+","VBusOn","SCK1","XTAL1","XTAL2"};
char *fixedPins[FIXED_COUNT] = {"RPA8","RPB15","RPB8","RPB3","RPB9","RPB2","RPB11","RPB5","RPB10","RPB14","RPB14","RPA2","RPA3"};

