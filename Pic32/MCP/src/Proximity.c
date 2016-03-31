/*
 * File:   Proximity.c
 * Author: martin
 *
 * Created on April 12, 2015
 */

//File to hold the Proximity data structure and reporting code
//Structures updated by I2C1_Task (Sonar), Analog_Task (IR & FSR), I2C4_Task (PIR)

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define _SUPPRESS_PLIB_WARNING
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING
#include "plib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSubData.h"
#include "PubSub/Notifications.h"

#include "MCP.h"

int proxConditionSet[CONDITION_COUNT];  //count to nest condition sets
int *proxEnable[PROX_COUNT] = PROX_ENABLE;

//reporting timer
TimerHandle_t proxReportTimer;
void ProximityReportTimerFired(TimerHandle_t xTimer);
void SendProximityReport(bool force);

static void ProximityReportingTask(void *pvParameters);

int ProximityInit() {
    int i;
    //initialize the proximity control data
    for (i = 0; i < PROX_COUNT; i++) {
        proxSensorData[i].status = PROX_OFFLINE;
        proxSensorData[i].direction = proxDirections[i];
        proxSensorData[i].directionMask = 0x1 << proxDirections[i];
        proxSensorData[i].currentRange = -1;
        proxSensorData[i].oldestHitTime = 0;
        proxSensorData[i].oldestMissTime = 0;
        proxSensorData[i].lastEventTime = 0;
        proxSensorData[i].eventNotified = false;
    }
    for (i = 0; i< CONDITION_COUNT; i++)
    {
        proxConditionSet[i] = 0;
    }
    
    /* Create the Notify task */
    if (xTaskCreate(ProximityReportingTask, /* The function that implements the task. */
            "Proximity", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
            PROXIMITY_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            PROXIMITY_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL) /* The task handle is not required, so NULL is passed. */
            != pdPASS) {

        LogError("Proximity Task create error");
        SetCondition(IR_PROXIMITY_ERROR);
        SetCondition(MCP_INIT_ERROR);
        return -1;
    }
    return 0;
}

//decide whether to send a Notification
void ProximityReview(ProximityDevices_enum proxDevice, bool _proxHit) {
    bool proxHit = _proxHit;
    int *enableVar = proxEnable[proxDevice];
    
    if (enableVar)
    {
        if (!*enableVar) proxHit = false;   //ignore hit from disabled sensor
    }
    
    ProxSensorData_t *thisSensor = &proxSensorData[proxDevice];
    if (proxHit) {
        //track duration of hit
        thisSensor->oldestMissTime = 0;
        if (!thisSensor->oldestHitTime) thisSensor->oldestHitTime = xTaskGetTickCount();
        //notify if its been active long enough
        if (!thisSensor->eventNotified) {
            if (thisSensor->oldestHitTime + proxHitDetection < xTaskGetTickCount()) {
                if (proxDeviceType[proxDevice] == BUMPER_FSR)
                {
                    if (proxDevice == FL_BUMPER || proxDevice == FR_BUMPER)
                    {
                        NotifyEvent(FRONT_CONTACT_EVENT);
                    }
                    else
                    {
                        NotifyEvent(REAR_CONTACT_EVENT);                        
                    }
                }
                else if ((proxDeviceType[proxDevice] == ANALOG_IR)
                        || (proxDeviceType[proxDevice] == DIGITAL_IR))
                {
                      NotifyEvent(PROXIMITY_EVENT);
                }
                else if (proxDeviceType[proxDevice] == PASSIVE_IR)
                {
                      NotifyEvent(PIR_EVENT);
                }
                thisSensor->lastEventTime = xTaskGetTickCount();
                thisSensor->eventNotified = true;

                if (proxConditionSet[proxConditions[proxDevice]]++ == 0)
                {
                    SetCondition(proxConditions[proxDevice]);
                }
            }
        }

    } else {
        //track duration of miss
        thisSensor->oldestHitTime = 0;
        if (!thisSensor->oldestMissTime) thisSensor->oldestMissTime = xTaskGetTickCount();
        //cancel if its been absent long enough
        if (thisSensor->eventNotified) {
            if (thisSensor->oldestMissTime + proxMissDetection < xTaskGetTickCount()) 
            {
                thisSensor->eventNotified = false;
                if (--proxConditionSet[proxConditions[proxDevice]] <= 0) 
                {
                    //cancel if no other sensors active
                    CancelCondition(proxConditions[proxDevice]);
                }

            }
        }
    }
}

//called periodically to send a proximity summary message if needed
static psMessage_t ProxMessage;
void SendProximityReport(bool force) {
    int i;
    int sensorRange, thisDirection;
    uint8_t thisStatus;
    ProxStatus_enum newProxStatus[PROX_SECTORS];    //working copy changed status
    
    for (i=0; i<PROX_SECTORS; i++)
    {
        newProxStatus[i] = PROX_OFFLINE;    //default
    }

    psInitPublish(ProxMessage, PROXREP);
    
    //review each proximity sensor
    for (i = 0; i < PROX_COUNT; i++) {
        thisDirection   = proxSensorData[i].direction;
        thisStatus      = newProxStatus[thisDirection];
        sensorRange     = proxSensorData[i].currentRange;
        
        //check sensor operational status
        switch (proxSensorData[i].status)
        {
            case PROX_OFFLINE:
                continue;
                break;
            case PROX_ERRORS:
                if (thisStatus < PROX_ERRORS) newProxStatus[thisDirection] = PROX_ERRORS;
                continue;
                break;
            case PROX_ACTIVE:
                if (thisStatus < PROX_ACTIVE) thisStatus = newProxStatus[thisDirection] = PROX_ACTIVE;
                break;
            default:
                break;
        }
        if (sensorRange < 0)
        {
//            if (thisStatus < PROX_ERRORS) newProxStatus[thisDirection] = PROX_ERRORS;
            continue; //no data for some reason
        }
        
        //evaluate range data
        switch (proxDeviceType[i]) {
            default:
                break;
            case VIRTUAL_FSR:
                //combine preceding and following FSR
                if (proxSensorData[i - 1].currentRange &&
                        proxSensorData[i + 1].currentRange) {
                    newProxStatus[thisDirection] = PROX_CONTACT;
                }
                break;
            case BUMPER_FSR:
                if (sensorRange) {
                    newProxStatus[thisDirection] = PROX_CONTACT;
                }
                break;
            case MB7070_SONAR:
                if (sensorRange < maxRangeSonar && sensorRange > minRangeSonar ) {
                    if (thisStatus < PROX_FAR) newProxStatus[thisDirection] = PROX_FAR;
                }
                break;
            case ANALOG_IR:

                if (sensorRange < maxRangeIR) {
                    if (thisStatus < PROX_CLOSE) newProxStatus[thisDirection] = PROX_CLOSE;
                }
                break;
            case DIGITAL_IR:
                if (proxSensorData[i].currentRange) {
                    if (thisStatus < PROX_CLOSE) newProxStatus[thisDirection] = PROX_CLOSE;
                }
                break;
            case PASSIVE_IR:
                 if (proxSensorData[i].currentRange) {
                    if (thisStatus < PROX_PASSIVE) newProxStatus[thisDirection] = PROX_PASSIVE;
                }
               break;
        }
    }

    bool statusChange = false;
    for (i=0; i<PROX_SECTORS; i++)
    {
        if (newProxStatus[i] != ProxMessage.proxSummaryPayload.proxStatus[i])
        {
            statusChange = true;
            ProxMessage.proxSummaryPayload.proxStatus[i] = newProxStatus[i];
        }
    }
    
    if (statusChange || force) psSendMessage(ProxMessage);
}
static void ProximityReportingTask(void *pvParameters)
{
    while (1)
    {
        vTaskDelay(proxReportIntervalMs);
        SendProximityReport(false);
    }
}
//the individual proximity devices
//Names
char *proxDeviceNames[PROX_COUNT] = PROX_DEVICE_NAMES;
//Direction covered
psProxDirection_enum proxDirections[PROX_COUNT] = PROX_DIRECTIONS;
//Notification raised
Condition_enum proxConditions[PROX_COUNT]   = PROX_CONDITIONS;
//Device type
ProximityDeviceType_enum proxDeviceType[PROX_COUNT] = PROXIMITY_DEVICE_TYPES;
//Minimum useful range
int minProxRangeByType[PROX_DEVICE_TYPE_COUNT] = MIN_RANGE_BY_TYPE;
//Maximum useful range
int maxProxRangeByType[PROX_DEVICE_TYPE_COUNT] = MAX_RANGE_BY_TYPE;

//current readings, etc.
ProxSensorData_t proxSensorData[PROX_COUNT];