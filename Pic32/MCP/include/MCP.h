/* 
 * File:   MCP.h
 * Author: martin
 *
 * Created on April 17, 2014, 7:40 PM
 */

#ifndef MCP_H
#define	MCP_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define _SUPPRESS_PLIB_WARNING

#include "FreeRTOS.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSubData.h"

#include "PMIC_msg.h"

#include "Drivers/Config/Config.h"

//PROX CONTROL STRUCTURE

//status of the individual proximity devices (includes Sonar, IR, FSR & PIR)

typedef struct {
    int direction;
    int currentRange;
    ProxStatus_enum status;
    TickType_t oldestHitTime;
    TickType_t oldestMissTime;
    TickType_t lastEventTime;
    bool eventNotified;
    uint8_t directionMask;
} ProxSensorData_t;

extern ProxSensorData_t proxSensorData[PROX_COUNT];

//Fixed data
//proximity device names
extern char *proxDeviceNames[PROX_COUNT];
//proximity device types
extern ProximityDeviceType_enum proxDeviceType[PROX_COUNT];
//proximity device directions
extern psProxDirection_enum proxDirections[PROX_COUNT];
//proximity device notifications
extern Event_enum proxEvents[PROX_COUNT];
extern Condition_enum proxConditions[PROX_COUNT];
//proximity device min range indexed by type
extern int minProxRangeByType[PROX_DEVICE_TYPE_COUNT];
//proximity device max range by type
extern int maxProxRangeByType[PROX_DEVICE_TYPE_COUNT];

//battery status
extern BatteryStatus_enum batteryStatus;

//environmental measurements
extern float ext_temp, int_temp, humidity, ambientLight,
        batteryVolts, batteryAmps, maxAmps, batteryAh, solarVolts, solarAmps, chargeVolts, chargeAmps;
extern bool motorInhibit;
extern bool isRaining;
#define RAIN_DELAY 10000        //wait to declare "it's raining"
#define MOTOR_INH_DELAY 250     //wait to set motor inhibit
//environment report control
extern TickType_t environmentNextReportTime;

//power state changes
extern PowerState_enum systemPowerState; //current power state
extern char *stateNames[];
//command from User
extern UserCommand_enum AppPowerCommand;
extern char *appCmdnames[];

extern OvermindPowerCommand_enum OvermindPowerCommand;
extern char *ovmCmdNames[];

extern bool OVMonline;
extern bool MOTonline;
extern bool APPonline;
extern bool *onlineCondition[PS_UARTS_COUNT];

extern NotificationMask_t WakeEventMask;

//RTC DS1307
//RTC EEPROM
typedef struct {
    TickType_t RTCreadTime;

    union {
        uint8_t rtcBytes[64];
        struct {
            uint8_t clockHalt : 1;
            uint8_t tenSeconds : 3;
            uint8_t seconds : 4;
            uint8_t tenMinutes : 4;
            uint8_t minutes : 4;
            uint8_t twelveHour : 2;
            uint8_t pm : 1;
            uint8_t tenHour : 1;
            uint8_t hours : 4;
            uint8_t day : 8;
            uint8_t tenMonth : 4;
            uint8_t month : 4;
            uint8_t tenYear : 4;
            uint8_t year : 4;
            uint8_t controlReg : 8;
            uint8_t sram[56];
        };
    };
} RtcTimeDate_t;
extern RtcTimeDate_t rtcTimeDate;

//PMIC
PMICstate_enum getPMICstate();
void PMIC_Command(PMICcommand_enum PMICcommand);

//power control
int PowerControlInit();
void ProcessOvermindCommand(OvermindPowerCommand_enum overmindCommand);
void ProcessUserCommand(UserCommand_enum userCommand);
void SetPowerState(PowerState_enum state);
bool isOvermindAwake();
bool StartOvermindPowerup(PowerState_enum nextState);
bool StartOvermindPoweroff(PowerState_enum nextState);

bool MotorInhibit(bool inhibit);

//task init prototypes
int Analog_TaskInit();
int I2C1_TaskInit();
int I2C4_TaskInit();
int Tick_TaskInit();

//RTC access
int RTCRead();
int RTCWrite();

//MCP task message hook
int MCPTaskProcessMessage(psMessage_t *msg, TickType_t wait);

//proximity
void SendProximityReport(bool force);
int ProximityInit();

#endif	/* MCP_H */

