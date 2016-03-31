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

NotificationMask_t ActiveConditions;
NotificationMask_t ReportedConditions;
NotificationMask_t ValidConditions;

SemaphoreHandle_t conditionMutex;   //Access to ActiveConditions

TickType_t lastSetTime[CONDITION_COUNT];
TickType_t lastCanceledTime[CONDITION_COUNT];

//TimerHandle_t ConditionReportingTimer;
//void ConditionTimerCallback(TimerHandle_t handle);

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
    
    ValidConditions = ReportedConditions = ActiveConditions = 0;

    //clear all timestamps
    for (i = 0; i< EVENT_COUNT; i++)
    {
        lastNotifiedTime[i] = 0;
    }
    for (i = 0; i< CONDITION_COUNT; i++)
    {
        lastSetTime[i] = 0;
        lastCanceledTime[i] = 0;
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

    LogRoutine("Notify: %s", eventNames[e]);

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
void SetCondition(Condition_enum e)
{
    if (e <= 0 || e >= CONDITION_COUNT) return;

    if (ActiveConditions & NOTIFICATION_MASK(e)) return;
        
    xSemaphoreTake(conditionMutex, 1000);
    ActiveConditions |= NOTIFICATION_MASK(e);
    ValidConditions |= NOTIFICATION_MASK(e);
    lastSetTime[e] = xTaskGetTickCount();
    xSemaphoreGive(conditionMutex);
    
    LogRoutine("Set: %s", conditionNames[e]);
}

void CancelCondition(Condition_enum e)
{
    if (e <= 0 || e >= CONDITION_COUNT) return;
    
    if (!(ActiveConditions & NOTIFICATION_MASK(e))) return;
    
    xSemaphoreTake(conditionMutex, 1000);
    ActiveConditions &= ~NOTIFICATION_MASK(e);
    ValidConditions |= NOTIFICATION_MASK(e);
    lastCanceledTime[e] = xTaskGetTickCount();
    xSemaphoreGive(conditionMutex);
    
    LogRoutine("Cancel: %s", conditionNames[e]);
}

bool isConditionActive(Condition_enum e)
{
    return (ActiveConditions & NOTIFICATION_MASK(e));
}

NotificationMask_t GetActiveConditions()
{
    NotificationMask_t c;
    
    xSemaphoreTake(conditionMutex, 1000);
    c = ActiveConditions;
    xSemaphoreGive(conditionMutex);
    
    return c;
}

TickType_t GetConditionLastSetTime(Condition_enum e)
{
    if (e <= 0 || e >= CONDITION_COUNT) return 0;
    xSemaphoreTake(conditionMutex, 1000);
    TickType_t t = lastSetTime[e];
    xSemaphoreGive(conditionMutex);
    return t;
}

TickType_t GetConditionLastCancelTime(Condition_enum e)
{
    if (e <= 0 || e >= CONDITION_COUNT) return 0;
    xSemaphoreTake(conditionMutex, 1000);
    TickType_t t = lastCanceledTime[e];
    xSemaphoreGive(conditionMutex);
    return t;
}

void PublishConditions(bool force) {
    psMessage_t msg;
    
    if ((ReportedConditions != ActiveConditions) || force)
    {  
        psInitPublish(msg, CONDITIONS);
        xSemaphoreTake(conditionMutex, 1000);
        msg.eventMaskPayload.value = ReportedConditions = ActiveConditions;
        msg.eventMaskPayload.valid = ValidConditions;
        xSemaphoreGive(conditionMutex);
        psSendMessage(msg);  
    }
}

static void ConditionReportingTask(void *pvParameters) {
    while(1)
    {
        vTaskDelay(CONDITION_REPORT_INTERVAL);
        PublishConditions(false);
    }
}