/*
 * File:   SystemStats.c
 * Author: martin
 *
 * Created on March 27, 2014
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
#include "Drivers/Serial/Serial.h"

#include "Messages.h"
#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"

void CONFIGURE_TIMER_FOR_RUN_TIME_STATS() {
    OpenTimer45(T45_ON | T45_IDLE_STOP | T45_PS_1_256, 0xffffffff);
}

uint32_t GET_RUN_TIME_COUNTER_VALUE() {
    return ReadTimer45();
}

void GenerateRunTimeTaskStats() {
    TaskStatus_t pxTaskStatusArray[SUBSYSTEM_RTOS_TASKS];
    volatile UBaseType_t uxArraySize, x, i;
    uint32_t ulTotalRunTime;
    uint32_t ulStatsAsPercentage;
    psMessage_t msg;

    // Take a snapshot of the number of tasks in case it changes while this
    // function is executing.
    uxArraySize = uxTaskGetNumberOfTasks();

    if (uxArraySize > SUBSYSTEM_RTOS_TASKS) {
        LogError("%i tasks", uxArraySize);
        return;
    }
    // Generate raw status information about each task.
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, SUBSYSTEM_RTOS_TASKS, &ulTotalRunTime);

    // For percentage calculations.
    ulTotalRunTime /= 100UL;

    // Avoid divide by zero errors.
    if (ulTotalRunTime > 0) {
        for (x = 0; x < uxArraySize; x++) {
            // What percentage of the total run time has the task used?
            // This will always be rounded down to the nearest integer.
            // ulTotalRunTime has already been divided by 100.
            ulStatsAsPercentage =  pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalRunTime;

            psInitPublish(msg, TASK_STATS);

            if (pxTaskStatusArray[ x ].pcTaskName) {
                strncpy(msg.taskStatsPayload.taskName, pxTaskStatusArray[ x ].pcTaskName, PS_NAME_LENGTH);
            } else {
                strcpy(msg.taskStatsPayload.taskName, "unknown");
            }
            msg.taskStatsPayload.percentCPU = (uint8_t) ulStatsAsPercentage;
            msg.taskStatsPayload.stackHeadroom = pxTaskStatusArray[ x ].usStackHighWaterMark;

            psSendMessage(msg);
            vTaskDelay(100);
        }
    }
}

void GenerateRunTimeSystemStats() {
    TaskStatus_t pxTaskStatusArray[SUBSYSTEM_RTOS_TASKS];
    volatile UBaseType_t uxArraySize, x;
    uint32_t ulTotalRunTime;
    uint32_t ulTaskRunTime = 0;
    uint32_t ulStatsAsPercentage = 0;
    psMessage_t msg;

    // Take a snapshot of the number of tasks in case it changes while this
    // function is executing.
    uxArraySize = uxTaskGetNumberOfTasks();

    if (uxArraySize <= SUBSYSTEM_RTOS_TASKS) {
        TaskHandle_t idleTask = xTaskGetIdleTaskHandle( );
        // Generate raw status information about each task.
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, SUBSYSTEM_RTOS_TASKS, &ulTotalRunTime);

        // Avoid divide by zero errors.
        if (ulTotalRunTime > 0) {
            for (x = 0; x < uxArraySize; x++) {
                if (pxTaskStatusArray[ x ].xHandle == idleTask) continue;
                
                ulTaskRunTime += pxTaskStatusArray[ x ].ulRunTimeCounter;
            }
            ulStatsAsPercentage = ((ulTaskRunTime * 100) / ulTotalRunTime);
        }
    }
    psInitPublish(msg, SYS_STATS);

    msg.sysStatsPayload.freeHeap = xPortGetFreeHeapSize();
    msg.sysStatsPayload.cpuPercentage = (uint8_t) ulStatsAsPercentage;

    psSendMessage(msg);
    vTaskDelay(100);
}
