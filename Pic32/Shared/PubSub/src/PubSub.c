/*
 * File:   PubSub.c
 * Author: martin
 *
 * Created on November 17, 2013, 6:17 PM
 */
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSubData.h"

#ifdef USE_BROKER_QUEUE
//receiving queue for PubSub Broker messages
PubSubQueue_t psBrokerQueue;

static void psLocalBrokerTask(void *pvParameters);
#endif

int countOfLostMessages = 0;

int PubSubInit() {
#ifdef USE_BROKER_QUEUE
    /* Create the queue. */
    psBrokerQueue = xQueueCreate(PS_QUEUE_LENGTH, sizeof ( psMessage_t));

    if (psBrokerQueue != NULL) {
        
        /* Create the local broker task */
        if (xTaskCreate(psLocalBrokerTask, /* The function that implements the task. */
                (signed char *) "Broker", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                PS_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
                (void *) 0, /* The parameter passed to the task. */
                PS_TASK_PRIORITY, /* The priority assigned to the task. */
                NULL) /* The task handle is not required, so NULL is passed. */
                                != pdPASS)
        {
            LogError("Broker");
            return -1;
        }
    }
    else
    {
        LogError("BrokerQ");
        return -1;
    }

#endif
    return 0;
}
#ifdef USE_BROKER_QUEUE
//called to add a message to the Broker task queue
bool psSendMessageViaBroker(psMessage_t *msg) {

    int reply = xQueueSendToBack(psBrokerQueue, msg, 0);
    if (reply != pdTRUE) {
        DebugPrint("Broker Discarded %s", psLongMsgNames[msg->header.messageType]);
        countOfLostMessages++;
        return false;
    }
    else if (reply == pdTRUE)
    {
        return true;
    }
    else
    {
        LogError("xQSend: %i", reply);
        return false;
    }
}
//this task takes a queue of messages from the interfaces and delivers them
void psLocalBrokerTask(void *pvParameters) {
    psMessage_t msg;
    int reply;
    while (1) {
        //wait for a message
        reply = xQueueReceive(psBrokerQueue, &msg, portMAX_DELAY);
        if (reply == pdTRUE) {
             psForwardMessage(&msg);
        }
        else
        {
            LogError("xQReceive: %i", reply);
        }
    }
}
#endif
//PubSub messages
void psSendPubSubMessage(psMessage_t *msg) {
    //set my originating source
    msg->header.source = THIS_SUBSYSTEM;

#ifdef USE_BROKER_QUEUE
    psSendMessageViaBroker(msg);
#else
    psForwardMessage(msg, portMAX_DELAY);
#endif
}

