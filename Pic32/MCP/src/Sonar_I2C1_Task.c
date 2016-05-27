/*
 * File:   I2C1_Task.c
 * Author: martin
 *
 * Created on February 15, 2014
 */

//Task to handle the I2C1_ bus.
//Obtains sonar data and publishes it.
//Interfaces with the RTC and PMIC

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

#include "MCP.h"
#include "Drivers/I2C/I2C.h"

#define I2C1_BUFFER_SIZE 4  //max bytes in

//functions in this file
//the RTOS task
static void I2C1_Task(void *pvParameters);

//sonar control
int SonarStart(uint8_t i2cAddress);
int SonarRead(uint8_t i2cAddress);

//I2C1_ Data buffers
uint8_t writeData[I2C1_BUFFER_SIZE];
uint8_t readData[I2C1_BUFFER_SIZE];

//last driver result
I2C_StateMachine_enum I2C1_Result;

//logging control
bool I2C1_errorLogged = false;

uint8_t sonarIndex[2] = {FL_SONAR, FR_SONAR};
uint8_t sonarAddress[2] = {FL_SONAR_I2C_ADDRESS, FR_SONAR_I2C_ADDRESS};

uint8_t newDistant = 0;
uint8_t reportedDistant = 0; // notification active
uint8_t cancelDistant = 0; // expired

//static message buffer
psMessage_t SonarMessage;
psMessage_t RawMsg;

TickType_t dataNextReportTime; //next report on volts/amps etc.

int I2C1_TaskInit() {

    //start the task
    if (xTaskCreate(I2C1_Task, /* The function that implements the task. */
            (signed char *) "Sonar I2C", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
            I2C1_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            I2C1_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL) /* The task handle is not required, so NULL is passed. */
            != pdPASS) {
        LogError("I2C1_ Task");
        SetCondition(MCP_INIT_ERROR);
        SetCondition(SONAR_PROXIMITY_ERROR);
        return -1;
    }
    return 0;
}

static void I2C1_Task(void *pvParameters) {
    uint8_t sonarClose;
    uint8_t sonarDistant;

    bool sonarPowered = false;

    //initialize the I2C1_ module
    if (!I2C_Begin(SONAR_I2C)) {
        LogError("I2C1 I2C_Begin error");
        SetCondition(I2C_BUS_ERROR);
        SetCondition(SONAR_PROXIMITY_ERROR);
        vTaskSuspend(NULL);
    }

    //Initialise pins
    //SONAR READY GPIO
    PORTSetPinsDigitalIn(SONAR_LEFT_READY_IOPORT, SONAR_LEFT_READY_BIT);
    PORTSetPinsDigitalIn(SONAR_RIGHT_READY_IOPORT, SONAR_RIGHT_READY_BIT);

    dataNextReportTime = xTaskGetTickCount() + envReport;

    DebugPrint("I2C1: Task Up");

    for (;;) {

        //adjust power state
        if (sonarProxEnable) //setting
        {
            if (!sonarPowered) {
                PORTSetBits(SONAR_PWR_IOPORT, SONAR_PWR_BIT);
                vTaskDelay(200); //let power settle
                sonarPowered = true;
                SonarStart(sonarAddress[0]); //start one ranging
            }
        } else {
            if (sonarPowered) {
                PORTClearBits(SONAR_PWR_IOPORT, SONAR_PWR_BIT);
                sonarPowered = false;
                proxSensorData[FL_SONAR].status = PROX_OFFLINE;
                proxSensorData[FR_SONAR].status = PROX_OFFLINE;
            }
        }

        newDistant = 0;
        reportedDistant = 0; // notification active
        cancelDistant = 0; // expired

        if (sonarPowered) {
            int i, source;

            for (i = 0; i < 2; i++) {
                //for each of left and right sonar (operated alternately)
                int proxRange;

                vTaskDelay(50); //TODO Check ready signal with shorter delay

                source = sonarIndex[i]; //index into proxSensorData array

                proxRange = SonarRead(sonarAddress[i]); //get the data

                SonarStart(sonarAddress[1 - i]); //start the other ranging

                proxSensorData[source].currentRange = proxRange;

                if (proxRange < 0) {
                    proxSensorData[source].status = PROX_ERRORS;
                    continue;
                }
                proxSensorData[source].status = PROX_ACTIVE;

                ProximityReview(source, (proxRange < maxRangeSonar && proxRange > minRangeSonar));
            }

            if (dataNextReportTime < xTaskGetTickCount()) {
                if (rawProxReport)
                {
                    psInitPublish(RawMsg, INT_DATA);
                    strncpy(RawMsg.nameIntPayload.name, proxDeviceNames[FL_SONAR], PS_NAME_LENGTH);
                    RawMsg.nameIntPayload.value = proxSensorData[FL_SONAR].currentRange;
                    psSendMessage(RawMsg);
                    vTaskDelay(50);
                    psInitPublish(RawMsg, INT_DATA);
                    strncpy(RawMsg.nameIntPayload.name, proxDeviceNames[FR_SONAR], PS_NAME_LENGTH);
                    RawMsg.nameIntPayload.value = proxSensorData[FR_SONAR].currentRange;
                    psSendMessage(RawMsg);
                }
                dataNextReportTime = xTaskGetTickCount() + envReport;
            }
        } else vTaskDelay(100);
    }
}

int SonarStart(uint8_t i2cAddress) {
    writeData[0] = 0x51; //take a reading command

    I2C1_Result = I2C_Write(SONAR_I2C, i2cAddress, writeData, 1);
    if (I2C1_Result != I2C_OK) {
        if (!I2C1_errorLogged) {
            DebugPrint("I2C1: SonarStart %2x: %s", i2cAddress, I2C_errorMsg(I2C1_Result));
            I2C1_errorLogged = true;
            SetCondition(I2C_BUS_ERROR);
            SetCondition(SONAR_PROXIMITY_ERROR);
        }
    } else {
        I2C1_errorLogged = false;
    }
    return I2C1_Result;
}

int SonarRead(uint8_t i2cAddress) {

    I2C1_Result = I2C_Read(SONAR_I2C, i2cAddress, writeData, 0, readData, 2);
    if (I2C1_Result == I2C_OK) {
        I2C1_errorLogged = false;
        return ((readData[0] << 8) | readData[1]);
    } else {
        if (!I2C1_errorLogged) {
            DebugPrint("I2C1: SonarRead %2x: %s", i2cAddress, I2C_errorMsg(I2C1_Result));
            I2C1_errorLogged = true;
            SetCondition(I2C_BUS_ERROR);
            SetCondition(SONAR_PROXIMITY_ERROR);
        }
        return -1;
    }
}

//RTC Chip

bool RTCerrorLogged = false;
int RTCRead() {
    char writeData;
    I2C_StateMachine_enum I2C1_Result = I2C_Read(SONAR_I2C, RTC_I2C_ADDRESS, &writeData, 0, rtcTimeDate.rtcBytes, 64);
    if (I2C1_Result == I2C_OK) {
        rtcTimeDate.RTCreadTime = xTaskGetTickCount();
        RTCerrorLogged = false;
    }
    else {
        if (!RTCerrorLogged) {
            DebugPrint("I2C1: RTC R: %s", I2C_errorMsg(I2C1_Result));
            SetCondition(I2C_BUS_ERROR);
            RTCerrorLogged = true;
        }
    }
    return I2C1_Result;
}

int RTCWrite() {

    I2C_StateMachine_enum I2C1_Result = I2C_Write(SONAR_I2C, RTC_I2C_ADDRESS, rtcTimeDate.rtcBytes, 64);
    if (I2C1_Result == I2C_OK) {
        rtcTimeDate.RTCreadTime = 0;
        RTCerrorLogged = false;
    } else {
        if (!RTCerrorLogged) {
            DebugPrint("I2C1: RTC W: %s", I2C_errorMsg(I2C1_Result));
            SetCondition(I2C_BUS_ERROR);
            RTCerrorLogged = true;
        }
    }
    return I2C1_Result;
}

RtcTimeDate_t rtcTimeDate;
