/*
 * File:   Notifications.c
 * Author: martin
 *
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "SoftwareProfile.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSubData.h"
#include "PubSub/Notifications.h"

//-------------------------------Events - triggered by changes
//timestamp to limit rate of messages
TickType_t lastNotifiedTime[EVENT_COUNT];

//--------------------------------Conditions - set and canceled

typedef enum {ACTIVE_MASK, VALID_MASK} Mask_enum;

NotificationMask_t active[MASK_PAYLOAD_COUNT];
NotificationMask_t reported[MASK_PAYLOAD_COUNT];
NotificationMask_t valid[MASK_PAYLOAD_COUNT];

SemaphoreHandle_t conditionMutex;   //Access to ActiveConditions

//--------------------------------Notify task and q to process events

QueueHandle_t notifyQ;

typedef struct {
    int event;
} NotifyQueue_t;

static void psNotifyTask(void *pvParameters);
static void ConditionReportingTask(void *pvParameters);

extern float notifyMinInterval;

int NotificationsInit() {
    int i;

    //clear all timestamps
    for (i = 0; i< EVENT_COUNT; i++)
    {
        lastNotifiedTime[i] = 0;
    }

    for (i=0; i<MASK_PAYLOAD_COUNT; i++)
    {
        valid[i] = reported[i] = active[i] = 0;
    }
    
    //queue for Notify task
    
    notifyQ = xQueueCreate(NOTIFY_TASK_QUEUE_LENGTH, sizeof (NotifyQueue_t));
    if (notifyQ == NULL) {
        LogError("Failed to create Notify Q");
        return -1;
    }

    //mutex for condition mask
    
    conditionMutex = xSemaphoreCreateMutex();
    if( conditionMutex == NULL )
    {
        LogError("Failed to create Condition Mutex");
        return -1;
    }
    
    /* Create the Notify task */
    if (xTaskCreate(psNotifyTask, /* The function that implements the task. */
            "Notify", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
            NOTIFY_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            NOTIFY_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL) /* The task handle is not required, so NULL is passed. */
            != pdPASS) {

        LogError("Notify Task create error");
        return -1;
    }
    
    /* Create the Notify task */
    if (xTaskCreate(ConditionReportingTask, /* The function that implements the task. */
            "Conditions", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
            CONDITION_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            CONDITION_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL) /* The task handle is not required, so NULL is passed. */
            != pdPASS) {

        LogError("Conditions Task create error");
        return -1;
    }
    return 0;
}

//processes notifications and sends NOTIFY messages as needed
static void psNotifyTask(void *pvParameters) {
    NotifyQueue_t nq;
    psMessage_t msg;

    for (;;) {
        //wait for a message
        if (xQueueReceive(notifyQ, &nq, portMAX_DELAY) == pdTRUE) {

            NotificationMask_t notifyBit = NOTIFICATION_MASK(nq.event);

            if (lastNotifiedTime[nq.event] + (int)notifyMinInterval < xTaskGetTickCount())
            {               
                lastNotifiedTime[nq.event] = xTaskGetTickCount();

                psInitPublish(msg, NOTIFY);
                msg.intPayload.value = nq.event;
                psSendMessage(msg);
            }
        }
    }
}

//------------------------Events
void NotifyEvent(Event_enum e) {
    NotifyQueue_t nq;

    if (e <= 0 || e >= EVENT_COUNT) return;

    DebugPrint("Notify: %s", eventNames[e]);

    nq.event = e;

    if (xQueueSendToBack(notifyQ, &nq, 0) != pdTRUE) {
        LogError("Notify Q full");
    }
}
TickType_t GetEventLastNotifiedTime(Event_enum e)
{
    return lastNotifiedTime[e];
}

//--------------------Conditions
//--------------------Conditions
void SetMaskBit(Mask_enum m, Condition_enum e)
{
	int bit 	= e % 64;
	int index 	= e / 64;

	switch(m)
	{
	case ACTIVE_MASK:
		active[index] |= NOTIFICATION_MASK(e);
		break;
	case VALID_MASK:
		valid[index] |= NOTIFICATION_MASK(e);
		break;
	}
}

void ClearMaskBit(Mask_enum m, Condition_enum e)
{
	int bit 	= e % 64;
	int index 	= e / 64;

	switch(m)
	{
	case ACTIVE_MASK:
		active[index] &= ~NOTIFICATION_MASK(e);
		break;
	case VALID_MASK:
		valid[index] &= ~NOTIFICATION_MASK(e);
		break;
	}
}

bool isConditionActive(Condition_enum e)
{
	int bit 	= e % 64;
	int index 	= e / 64;

	return (active[index] & NOTIFICATION_MASK(e));
}

void SetCondition(Condition_enum e)
{
    if (e <= 0 || e >= CONDITION_COUNT) return;

    bool prior;   
    xSemaphoreTake(conditionMutex, 1000);
    prior = isConditionActive(e);
    SetMaskBit(ACTIVE_MASK, e);
    SetMaskBit(VALID_MASK, e);
    xSemaphoreGive(conditionMutex);
    
    if (!prior) DebugPrint("Set: %s", conditionNames[e]);
}

void CancelCondition(Condition_enum e)
{
    if (e <= 0 || e >= CONDITION_COUNT) return;
    
    bool prior;
    
    xSemaphoreTake(conditionMutex, 1000);
    prior = isConditionActive(e);
    ClearMaskBit(ACTIVE_MASK, e);
    SetMaskBit(VALID_MASK, e);
    xSemaphoreGive(conditionMutex);
    
    if (prior) DebugPrint("Cancel: %s", conditionNames[e]);
}

void PublishConditions(bool _force) {
    psMessage_t msg;
    bool sendit = _force;
    int i;

    xSemaphoreTake(conditionMutex, 1000);
       
    if (!_force) {
        for (i = 0; i < MASK_PAYLOAD_COUNT; i++) {
            if (active[i] != reported[i]) {
                sendit = true;
                break;
            }
        }
    }
    
    if (sendit) {
        psInitPublish(msg, CONDITIONS);

        for (i = 0; i < MASK_PAYLOAD_COUNT; i++) {
            msg.maskPayload.value[i] = active[i];
            msg.maskPayload.valid[i] = valid[i];
            reported[i] = active[i];
        }
        xSemaphoreGive(conditionMutex);
        psSendMessage(msg);
    }
      else xSemaphoreGive(conditionMutex);
}

static void ConditionReportingTask(void *pvParameters) {
    while(1)
    {
        vTaskDelay(CONDITION_REPORT_INTERVAL);
        PublishConditions(false);
    }
}