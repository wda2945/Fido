//
//  FubarinoMiniPPS.h
//  PPSconfig
//
//  Created by Martin Lane-Smith on 4/22/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#ifndef PPSconfig_FubarinoMiniPPS_h
#define PPSconfig_FubarinoMiniPPS_h

#define GROUP_COUNT 4
#define MAX_PPS_SIGNALS 8
#define PPS_PIN_COUNT 29
extern char *PPSPinName[PPS_PIN_COUNT];
extern char PPSGroup[PPS_PIN_COUNT];
extern char *PPSinSignals[GROUP_COUNT][MAX_PPS_SIGNALS];
extern char *PPSoutSignals[GROUP_COUNT][MAX_PPS_SIGNALS];

#define AN_COUNT 13
extern char *analogPins[AN_COUNT];

#define PIN_COUNT 31
extern char *connectorPins[PIN_COUNT];
extern char connectorNumber[PIN_COUNT];

#define FIXED_COUNT 13
extern char *fixedSignals[FIXED_COUNT];
extern char *fixedPins[FIXED_COUNT];

#define UART_COUNT 2
#define SPI_COUNT 2
#define I2C_COUNT 2
#define OC_COUNT 5
#define INT_COUNT 4

#endif
