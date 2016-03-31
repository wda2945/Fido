/* 
 * File:   PubSub.h
 * Author: martin
 *
 * Created on November 17, 2013, 6:17 PM
 */

/*
 Header file for Publish-Subscribe functionality.
 */

#ifndef PUBSUB_H
#define	PUBSUB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "PubSubData.h"
#include "SoftwareProfile.h"
#include "queue.h"


//queueing wait
#define PS_WAIT_TICKS   100             //QOS2 short wait for queue to clear

typedef QueueHandle_t PubSubQueue_t;

#ifdef USE_BROKER_QUEUE
//queue for local broker
extern PubSubQueue_t psBrokerQueue;
#endif

#define psNewPubSubQueue(entries) xQueueCreate((entries), sizeof(psMessage_t))

#ifndef THIS_SUBSYSTEM
#define THIS_SUBSYSTEM SUBSYSTEM_UNKNOWN
#endif

//convenience macros to handle the message struct

//Initialize the header
#define psInitPublish(msg, _msgType) msg.header.messageType=_msgType;msg.header.source=THIS_SUBSYSTEM;

//Fill out the payload

//send the message - used by regular tasks
#define psSendMessage(msg) psSendPubSubMessage(&msg)

//Publish with no payload
#define psPublish(M) {psMessage_t subMsg;\
    subMsg.header.messageType=M;\
    psSendPubSubMessage(&subMsg);}

    //PubSub Initialization
    int PubSubInit();

    void AdjustMessageLength(psMessage_t *_msg);

    bool psSendMessageViaBroker(psMessage_t *msg);

    //send a message to subscribers
    void psSendPubSubMessage(psMessage_t *msg);

    //forward without changing source code
    void psForwardMessage(psMessage_t *msg, TickType_t wait);
    
#if (PS_UARTS_COUNT > 0)
    bool UARTProcessMessage(Subsystem_enum sourceCode, psMessage_t *msg, TickType_t wait);
    int UARTBrokerInit();
    void UARTSendStats();
#endif
#ifdef PS_BLE_BROKER
    bool BLEProcessMessage(psMessage_t *msg, TickType_t wait);
    int BLEBrokerInit();
    void BLESendStats();
#endif
#ifdef XBEE_BROKER
    bool XBeeBrokerProcessMessage(psMessage_t *msg, TickType_t wait);    
    int XBeeBrokerInit();
    void XBeeSendStats();
#endif
    
    
#endif	/* PUBSUB_H */

