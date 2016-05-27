/*
 * File:   BLEBroker.c
 * Author: martin
 *
 * Created on November 22, 2013
 */

//Forwards PubSub messages via BLE to the iOS App

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define _SUPPRESS_PLIB_WARNING
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING

#include "plib.h"

#include "FreeRTOS.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSubData.h"

#include "Helpers.h"
#include "Drivers/Serial/Serial.h"
#include "MCP.h"

#include "task.h"
#include "semphr.h"
#include "timers.h"

#define MAX_BLE_MESSAGE 16
#define SEMAPHORE_WAIT 50

static void psBLERxTask(void *pvParameters);
static void psBLETxTask(void *pvParameters);

PubSubQueue_t bleQueue; //Queue for messages to be sent over the BLE
SemaphoreHandle_t bleMutex; //Mutex to separate send and receive

int bleRoutine, bleInfo, bleWarning, bleError;

TimerHandle_t bleTimeout;
void bleOfflineTimerCallback(TimerHandle_t xTimer);

//statistics collection
TickType_t BLElastClearedTime;
//TX
int BLEmessagesSent;
int BLEaddressDiscarded;
int BLEcongestionDiscarded;      //congestion
int BLElogMessagesDiscarded;
//RX
int BLEmessagesReceived;
int BLEaddressIgnored;        //wrong address
int BLEparseErrors;

int bleQueueLimits[] = BLE_QUEUE_LIMITS;

bool BLEProcessMessage(psMessage_t *msg, TickType_t wait) {
    if (!bleQueue || !useBLE) return false;

    if ((msg->header.source == APP_XBEE) || (msg->header.source == APP_OVM))
    {
        BLEaddressDiscarded++;
        return false;
    }
    bool discard = false;
    //only send messages not App-sourced
    if (msg->header.messageType == SYSLOG_MSG) {
        //throttle log messages
        switch (msg->logPayload.severity) {
            case SYSLOG_ROUTINE:
                if (bleRoutine > (int)maxAPPRoutine) discard = true;
                else ++bleRoutine;
                break;
            case SYSLOG_INFO:
                if (bleInfo > (int)maxAPPInfo) discard = true;
                else ++bleInfo;
                break;
            case SYSLOG_WARNING:
                if (bleWarning > (int)maxAPPWarning) discard = true;
                else ++bleWarning;
                break;
            case SYSLOG_ERROR:
                if (bleError > (int)maxAPPError) discard = true;
                else ++bleError;
            default:
                break;
        }
    }
    if (discard)
    {
        BLElogMessagesDiscarded++;
        return false;
    }
    //check q space
    int waiting = 0;//uxQueueMessagesWaiting(bleQueue);

    if (waiting >= bleQueueLimits[psQOS[ msg->header.messageType]]) {
        SetCondition(MCP_APP_COMMS_CONGESTION);
        BLEcongestionDiscarded++;
        DebugPrint("BLE Discarded %s", psLongMsgNames[msg->header.messageType]);
        return false;
    }

    if (xQueueSendToBack(bleQueue, msg, wait)  != pdTRUE) {
        SetCondition(MCP_APP_COMMS_CONGESTION);
        BLEcongestionDiscarded++;
        DebugPrint("BLE Q Full: %s", psLongMsgNames[msg->header.messageType]);
        return false;
    }
    return true;
}

int BLEBrokerInit(psTopic_enum topicList[]) {

    bleRoutine = bleInfo = bleWarning = bleError = 0;

    //queue for messages to be sent to App
    if ((bleQueue = psNewPubSubQueue(BLE_QUEUE_LENGTH)) == NULL) {
        LogError("BLE Queue");
        return -1;
    }
    //semaphore to separate send from receive
    if ((bleMutex = xSemaphoreCreateMutex()) == NULL) {
        LogError("BLE Mutex");
        return -1;
    }

    //create the offline timer
    bleTimeout = xTimerCreate("BLEOffline", // Just a text name, not used by the kernel.
            BLE_OFFLINE_TIMER_PERIOD, // The timer period in ticks.
            pdFALSE, // The timer will auto-reload itself when it expires.
            (void *) 0,
            bleOfflineTimerCallback
            );

    //start the uart
    if (Serial_begin(BLE_UART, BLE_BAUDRATE, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1,
            BLE_BROKER_BUFFER_SIZE, BLE_BROKER_BUFFER_SIZE)) {

        /* Create the BLE Tx task */
        if ((xTaskCreate(psBLETxTask, /* The function that implements the task. */
                (signed char *) "BLE Tx", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                BLE_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
                (void *) 0, /* The parameter passed to the task. */
                BLE_TASK_PRIORITY, /* The priority assigned to the task. */
                NULL)) /* The task handle is not required, so NULL is passed. */
                != pdPASS) {
            LogError("BLE Tx Task");
            return -1;
        }
        /* Create the BLE Rx task */
        if ((xTaskCreate(psBLERxTask, /* The function that implements the task. */
                (signed char *) "BLE Rx", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                BLE_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
                (void *) 0, /* The parameter passed to the task. */
                BLE_TASK_PRIORITY, /* The priority assigned to the task. */
                NULL)) /* The task handle is not required, so NULL is passed. */
                != pdPASS) {
            LogError("BLE Rx Task");
            return -1;
        }
    } else {
        LogError("BLE UART");
        return -1;
    }
    return 0;
}

//Tx task - sends on (most) anything it receives

static void psBLETxTask(void *pvParameters) {
    psMessage_t msg;
    bool holdMutex = false;
    char encodedMessage[MAX_ENCODED_MESSAGE];

    DebugPrint("BLE Tx Up");

    for (;;) {

        //wait for a message
        xQueueReceive(bleQueue, &msg, portMAX_DELAY);

        if (msg.header.messageType == SYSLOG_MSG) {
            switch (msg.logPayload.severity) {
                case SYSLOG_ROUTINE:
                    bleRoutine--;
                    break;
                case SYSLOG_INFO:
                    bleInfo--;
                    break;
                case SYSLOG_WARNING:
                    bleWarning--;
                    break;
                default:
                    bleError--;
                    break;
            }
        } else {
#ifdef BLE_DEBUG_LOG
            DebugPrint("BLE Tx: %s", psLongMsgNames[msg.header.messageType]);
#endif
        }

        int encodedLen = msgToText(&msg, encodedMessage, MAX_ENCODED_MESSAGE);

        char *next = encodedMessage;
        int bytesToSend;

        do {
            if (encodedLen > MAX_BLE_MESSAGE) {
                bytesToSend = MAX_BLE_MESSAGE;
                encodedLen -= bytesToSend;
            } else {
                bytesToSend = encodedLen;
                encodedLen = 0;
            }

            if (xSemaphoreTake(bleMutex, SEMAPHORE_WAIT) == pdTRUE) holdMutex = true;
            else holdMutex = false;

            vTaskDelay(50 / portTICK_PERIOD_MS);

            while (bytesToSend) {
                Serial_write(BLE_UART, *next++);
                bytesToSend--;
            }
            Serial_flush(BLE_UART);
            BLEmessagesSent++;
            
            vTaskDelay(20 / portTICK_PERIOD_MS);

            if (holdMutex) xSemaphoreGive(bleMutex);

        } while (encodedLen);

    }
}
psMessage_t bleRxMsg;
static void psBLERxTask(void *pvParameters) {
    
    unsigned char encodedMessage[MAX_ENCODED_MESSAGE];
    unsigned char *buffer;
    unsigned char nextChar;
    int size;
    bool holdMutex;
    UART_DATA dat;

    DebugPrint("BLE Rx Up");

    for (;;) {
        int messageReceived = 0;

        buffer = encodedMessage;
        size = MAX_ENCODED_MESSAGE;

        dat = Serial_read(BLE_UART);
        nextChar = dat.data8bit;

        if (xSemaphoreTake(bleMutex, SEMAPHORE_WAIT) == pdTRUE) holdMutex = true;
        else holdMutex = false;

        do {
            switch (nextChar) {
                default:
                    *buffer++ = nextChar;
                    size--;

                    dat = Serial_read(BLE_UART);
                    nextChar = dat.data8bit;

                    break;
                case '!':
                    //end of packet
                    *buffer = '\0';
                    if (textToMsg(encodedMessage, &bleRxMsg) == 0) {
                        bleRxMsg.header.source = APP_XBEE;

                        if (bleRxMsg.header.messageType != SYSLOG_MSG)
{
#ifdef BLE_DEBUG_LOG
                            DebugPrint("BLE Rx: %s", psLongMsgNames[bleRxMsg.header.messageType]);
#endif
                        }
                        
                        if (bleRxMsg.header.messageType == KEEPALIVE)
                        {
                            //send if back
                            BLEProcessMessage(&bleRxMsg, 0);
                        }
                        else
                        {
                            //otherwise just forward it
                            psForwardMessage(&bleRxMsg, 100);
                            BLEmessagesReceived++;
                        }
                        if (!APPonline)
                        {
                            APPonline = true;
                            NotifyEvent(APPONLINE_EVENT);
                        }
                        xTimerReset(bleTimeout, 0);
                    }
                    messageReceived = 1;
                    break;
                case '\r':
                case '\0':
                case '\n':
                    //ignore
                    dat = Serial_read(BLE_UART);
                    nextChar = dat.data8bit;
                    break;
            }
        } while (size && !messageReceived);

        if (holdMutex) xSemaphoreGive(bleMutex);
    }
}

void bleOfflineTimerCallback(TimerHandle_t xTimer) {
    APPonline = false;
}

void BLESendStats() {
    psMessage_t msg;
    
    BLElastClearedTime = xTaskGetTickCount();
    psInitPublish(msg, COMMS_STATS);

    strncpy(msg.commsStatsPayload.destination, "BLE", 4);
    msg.commsStatsPayload.messagesSent = BLEmessagesSent;
    msg.commsStatsPayload.addressDiscarded = BLEaddressDiscarded;
    msg.commsStatsPayload.congestionDiscarded = BLEcongestionDiscarded; //congestion
    msg.commsStatsPayload.logMessagesDiscarded = BLElogMessagesDiscarded;
    msg.commsStatsPayload.messagesReceived = BLEmessagesReceived;
    msg.commsStatsPayload.addressIgnored = BLEaddressIgnored; //wrong address
    msg.commsStatsPayload.parseErrors = BLEparseErrors; //wrong address

    BLEmessagesSent = 0;
    BLEaddressDiscarded = 0;
    BLEcongestionDiscarded = 0; //congestion
    BLElogMessagesDiscarded = 0;
    BLEmessagesReceived = 0;
    BLEaddressIgnored = 0; //wrong address
    BLEparseErrors = 0;

    psSendPubSubMessage(&msg);
}
