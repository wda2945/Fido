/***********************************************
 *  Hardware profile for MCP
 ***********************************************/

#ifndef _HARDWARE_PROFILE_H
#define _HARDWARE_PROFILE_H

#include <xc.h>

#define _SUPPRESS_PLIB_WARNING
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING
#include <plib.h>

#include "Fubarino.h"
#include "SoftwareProfile.h"

#define CLOCK_FREQ              80000000ul
#define GetSystemClock()        (CLOCK_FREQ)
#define GetPeripheralClock()    (CLOCK_FREQ/(1 << OSCCONbits.PBDIV))
#define GetInstructionClock()   (CLOCK_FREQ)

//MCP Pin Usage

#define SONAR_PWR_IOPORT  PIN_0_IOPORT
#define SONAR_PWR_BIT     PIN_0_BIT
//Pin 1 Sonar I2C1 SDA
//Pin 2 Sonar I2C1 SCL
#define SONAR_RIGHT_READY_IOPORT  PIN_3_IOPORT
#define SONAR_RIGHT_READY_BIT     PIN_3_BIT
#define SONAR_LEFT_READY_IOPORT  PIN_4_IOPORT
#define SONAR_LEFT_READY_BIT     PIN_4_BIT
//Pin 5
//Pin 6
#define IR_PROX_PWR_IOPORT  PIN_7_IOPORT
#define IR_PROX_PWR_BIT     PIN_7_BIT
//Pin 8 UART 1A RX from BLE
//Pin 9 UART 1A TX to BLE
#define BBB_BTN_IOPORT  PIN_10_IOPORT   //BBB Power Button
#define BBB_BTN_BIT     PIN_10_BIT
//#define PWR_BTN_PIN      "RPD4"
#define PWR_BTN_OD()    mPORTDOpenDrainOpen(BIT_4)
#define BBB_RST_IOPORT  PIN_11_IOPORT   //BBB Reset Line
#define BBB_RST_BIT     PIN_11_BIT
#define USING_PIN_11    1               //leave for input
//Pin 12
//Pin 13
//Pin 14
//Pin 15
#define PMUX3_IOPORT    PIN_16_IOPORT
#define PMUX3_BIT       PIN_16_BIT
#define PMUX2_IOPORT    PIN_17_IOPORT
#define PMUX2_BIT       PIN_17_BIT
#define PMUX1_IOPORT    PIN_18_IOPORT
#define PMUX1_BIT       PIN_18_BIT
#define PMUX0_IOPORT    PIN_19_IOPORT
#define PMUX0_BIT       PIN_19_BIT
#define AUX_PWR_IOPORT  PIN_20_IOPORT   //MCP23008, LED, PIR, Humidity  power
#define AUX_PWR_BIT     PIN_20_BIT
#define USER_LED_IOPORT PIN_21_IOPORT   //User LED on board
#define USER_LED_BIT    PIN_21_BIT
#define MRF_PWR_IOPORT  PIN_22_IOPORT  //Spare PIC power
#define MRF_PWR_BIT     PIN_22_BIT
//Pin 23
//Pin 24 PIC3 2B MOSI
//Pin 25 NAV 2A MISO
//Pin 26 NAV 2A MOSI
//Pin 27 PIC3 2B MISO
#define AMUX_IN_IOPORT  PIN_A14_IOPORT
#define AMUX_IN_BIT     PIN_A14_BIT
#define USING_PIN_A14   1
#define AMUX3_IOPORT    PIN_A13_IOPORT
#define AMUX3_BIT       PIN_A13_BIT
#define AMUX2_IOPORT    PIN_A12_IOPORT
#define AMUX2_BIT       PIN_A12_BIT
#define AMUX1_IOPORT    PIN_A11_IOPORT
#define AMUX1_BIT       PIN_A11_BIT
#define AMUX0_IOPORT    PIN_A10_IOPORT
#define AMUX0_BIT       PIN_A10_BIT
//Pin A9 ICSP
//Pin A8 ICSP
//Pin A7 UART 3B BBB MISO
#define PMUX_IN_IOPORT  PIN_A6_IOPORT
#define PMUX_IN_BIT     PIN_A6_BIT
#define USING_PIN_A6    1
#define CHARGE_IOPORT   PIN_A5_IOPORT
#define CHARGE_BIT      PIN_A5_BIT
#define SOLAR_IOPORT    PIN_A4_IOPORT
#define SOLAR_BIT       PIN_A4_BIT
//Pin A3
#define OVM_PWR_IOPORT  PIN_A2_IOPORT
#define OVM_PWR_BIT     PIN_A2_BIT
//Pin A1 UART 3B BBB MOSI
#define MOT_PWR_IOPORT  PIN_A0_IOPORT   //Motor proc power
#define MOT_PWR_BIT     PIN_A0_BIT
//Pin 28 UART 3A MOT MISO
//Pin 29 UART 3A MOT MOSI

//I2C1
#define FL_SONAR_I2C_ADDRESS 0x71   //7-bit addresses
#define FR_SONAR_I2C_ADDRESS 0x72
#define RTC_I2C_ADDRESS      0x68
//PMIC address in PMIC header

//I2C4
#define GPIO_I2C_ADDRESS	0x20
#define GPIO_IODIR              0x00
#define GPIO_GPPU               0x06
#define GPIO_PORT               0x09
enum {
	RIGHT_GREEN_LED = 0x01,
	REAR_RIGHT_LED  = 0x02,
	LEFT_PIR        = 0x04,
	FRONT_RIGHT_LED = 0x08,
	FRONT_LEFT_LED  = 0x10,
	REAR_LEFT_LED   = 0x20,
    LEFT_RED_LED    = 0x40,
	RIGHT_PIR       = 0x80
	} GPIOpins_enum;
#define NAV_LIGHTS (RIGHT_GREEN_LED + LEFT_RED_LED)
#define FRONT_LIGHTS (FRONT_RIGHT_LED + FRONT_LEFT_LED)
#define REAR_LIGHTS (REAR_RIGHT_LED + REAR_LEFT_LED)

#define AM2315_I2C_ADDRESS      0x6C

//PROXIMITY DEVICES
typedef enum {
    FL_SONAR,
    FR_SONAR,
    FL_BUMPER,
    FC_BUMPER,  //AND of right and left bumpers
    FR_BUMPER,
    FL_IR_PROX,
    FC_IR_PROX,
    FR_IR_PROX,
    RL_BUMPER,
    RC_BUMPER,
    RR_BUMPER,
    LEFT_IR_PROX,
    RIGHT_IR_PROX,
    RL_IR_PROX,
    RC_IR_PROX,
    RR_IR_PROX,
    LEFT_PASSIVE_IR,
    RIGHT_PASSIVE_IR,
    PROX_COUNT
} ProximityDevices_enum;

#define PROX_DEVICE_NAMES {\
    "Left_Sonar",\
    "Right_Sonar",\
    "FL_Bumper",\
    "FC_Bumper",\
    "FR_Bumper",\
    "FL_IR_Prox",\
    "FC_IR_Prox",\
    "FR_IR_Prox",\
    "RL_Bumper",\
    "RC_Bumper",\
    "RR_Bumper",\
    "Left_IR_Prox",\
    "Right_IR_Prox",\
    "RL_IR_Prox",\
    "RC_IR_Prox",\
    "RR_IR_Prox",\
    "Left PIR",\
    "Right PIR",\
}
//Proximity device types
typedef enum {
    MB7070_SONAR,
    BUMPER_FSR,
    VIRTUAL_FSR,
    ANALOG_IR,
    DIGITAL_IR,
    PASSIVE_IR,
    PROX_DEVICE_TYPE_COUNT
} ProximityDeviceType_enum;

#define MIN_RANGE_BY_TYPE {10,0,0,10,10}
#define MAX_RANGE_BY_TYPE {300,0,0,80,10}

#define PROXIMITY_DEVICE_TYPES {\
MB7070_SONAR,\
MB7070_SONAR,\
BUMPER_FSR,\
VIRTUAL_FSR,\
BUMPER_FSR,\
ANALOG_IR,\
ANALOG_IR,\
ANALOG_IR,\
BUMPER_FSR,\
VIRTUAL_FSR,\
BUMPER_FSR,\
ANALOG_IR,\
ANALOG_IR,\
DIGITAL_IR,\
DIGITAL_IR,\
DIGITAL_IR,\
PASSIVE_IR,\
PASSIVE_IR,\
}

//prox enable vars
#define PROX_ENABLE {\
    &sonarL, &sonarR, &bumpFL, 0, &bumpFR, &proxFL, &proxFC, &proxFR,\
    &bumpRL, 0, &bumpRR, &proxL, &proxR, &proxRear, &proxRear, &proxRear, &pirL, &pirR}
//relative direction of each proximity device
#define PROX_DIRECTIONS {\
    PROX_FRONT_LEFT, PROX_FRONT_RIGHT,\
    PROX_FRONT_LEFT, PROX_FRONT, PROX_FRONT_RIGHT, \
    PROX_FRONT_LEFT, PROX_FRONT, PROX_FRONT_RIGHT,\
    PROX_REAR_LEFT, PROX_REAR, PROX_REAR_RIGHT,\
    PROX_LEFT,  PROX_RIGHT,\
    PROX_REAR_LEFT, PROX_REAR, PROX_REAR_RIGHT,\
    PROX_LEFT, PROX_RIGHT,\
}
//notifications raised
#define PROX_CONDITIONS {\
FRONT_LEFT_FAR_PROXIMITY, FRONT_RIGHT_FAR_PROXIMITY,\
FRONT_CONTACT, FRONT_CONTACT, FRONT_CONTACT,\
FRONT_LEFT_PROXIMITY, FRONT_CENTER_PROXIMITY, FRONT_RIGHT_PROXIMITY,\
REAR_CONTACT, REAR_CONTACT, REAR_CONTACT,\
LEFT_PROXIMITY, RIGHT_PROXIMITY,\
REAR_LEFT_PROXIMITY, REAR_CENTER_PROXIMITY, REAR_RIGHT_PROXIMITY,\
LEFT_PASSIVE_PROXIMITY, RIGHT_PASSIVE_PROXIMITY,\
}

//Additional Non-proximity Analog Mux Sources
typedef enum {
    REAR_DIGITAL_PROX = PROX_COUNT, //combo digital IR sensor
    EXT_TEMP,
    AMBIENT_LIGHT,
    BATTERY_VOLTS,
    SOLAR_VOLTS,
    SOLAR_AMPS,
    CHARGE_VOLTS,
    CHARGE_AMPS,
    INT_TEMP,
    BATTERY_AMPS,
    RAIN_ANALOG,
    RAIN_DIGITAL,
    MOTOR_INH,
    NO_CONECTION,
    TOTAL_SOURCE_COUNT
} AMuxSource_enum;

#define AMUX_SOURCE_NAMES {\
    "Digital Prox",\
    "Ext Temp",\
    "Ambient Light",\
    "Battery Volts",\
    "Solar Volts",\
    "Solar Amps",\
    "Charger Volts",\
    "Charger Amps",\
    "Int Temp",\
    "Battery Amps",\
    "Analog Rain",\
    "Digital Rain",\
    "Motor Inhibit",\
    "NC"\
}

#define PMUX_CHANNELS {\
    NO_CONECTION,       /* Channel 0 */\
    NO_CONECTION,       /* Channel 1 */\
    NO_CONECTION,       /* Channel 2 */\
    REAR_DIGITAL_PROX,  /* Channel 3 */\
    LEFT_IR_PROX,       /* Channel 4 */\
    RIGHT_IR_PROX,      /* Channel 5 */\
    RR_BUMPER,          /* Channel 6 */\
    RL_BUMPER,          /* Channel 7 */\
    EXT_TEMP,           /* Channel 8 */\
    AMBIENT_LIGHT,      /* Channel 9 */\
    FL_BUMPER,          /* Channel 10 */\
    FR_BUMPER,          /* Channel 11 */\
    FR_IR_PROX,         /* Channel 12 */\
    FC_IR_PROX,         /* Channel 13 */\
    NO_CONECTION,       /* Channel 14 */\
    FL_IR_PROX,         /* Channel 15 */\
}

#define AMUX_CHANNELS {\
    MOTOR_INH,          /* Channel 0 */\
    CHARGE_AMPS,        /* Channel 1 */\
    CHARGE_VOLTS,       /* Channel 2 18k/198k divider */\
    SOLAR_AMPS,         /* Channel 3 */\
    SOLAR_VOLTS,        /* Channel 4 18k/198k divider */\
    BATTERY_VOLTS,      /* Channel 5 18k/198k divider */\
    RAIN_ANALOG,        /* Channel 6 47k/80k divider */\
    RAIN_DIGITAL,       /* Channel 7 47k/80k divider */\
    NO_CONECTION,       /* Channel 8 */\
    NO_CONECTION,       /* Channel 9 */\
    NO_CONECTION,       /* Channel 10 */\
    INT_TEMP,           /* Channel 11 */\
    NO_CONECTION,       /* Channel 12 */\
    NO_CONECTION,       /* Channel 13 */\
    BATTERY_AMPS,       /* Channel 14 */\
    NO_CONECTION        /* Channel 15 */\
}
#define BATTERY_VOLTS_DIVIDER   ((18.0f/198.0f) * (23.75f / 24.0f)) //corrected
#define SOLAR_VOLTS_DIVIDER     BATTERY_VOLTS_DIVIDER
#define CHARGER_VOLTS_DIVIDER   BATTERY_VOLTS_DIVIDER
#define AMPS_RANGE_FACTOR       1.0f

#define TEMP_0C_MILLIVOLTS      400
#define TEMP_MV_PER_C           19.5f

#define MUX_COUNT               16

void BoardInit(void);

#endif //_HARDWARE_PROFILE_H
