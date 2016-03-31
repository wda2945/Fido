/*
 * File:   Tick.c
 * Author: martin
 *
 */

//Generates 1 second heartbeat and broadcast system info

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "SoftwareProfile.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSubData.h"
#include "PubSub/Notifications.h"

#include "MCP.h"

static psMessage_t tickMsg;

//task fucntion
static void TickTask(void *pvParameters);

int Tick_TaskInit()
{
    //start the task
    if (xTaskCreate(TickTask, /* The function that implements the task. */
            (signed char *) "Tick", /* The text name assigned to the task*/
            TICK_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            TICK_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL) /* The task handle is not required, so NULL is passed. */
            != pdPASS) {
        LogError("Tick Task");
        SetCondition(MCP_INIT_ERROR);
        return -1;
    }
    return 0;
}

static void TickTask(void *pvParameters) {
    TickType_t xLastWakeTime;

    int commsStatsCount = commStatsSecs;
    
    psInitPublish(tickMsg, TICK_1S);
    xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        tickMsg.tickPayload.systemPowerState = systemPowerState;
        psSendMessage(tickMsg);

        if (--commsStatsCount <= 0)
        {
            commsStatsCount = commStatsSecs;
            if (commStats) 
            {
                UARTSendStats();
                BLESendStats();
                XBeeSendStats();
            }
        }
   
        vTaskDelayUntil(&xLastWakeTime, 1000);
    }
}

