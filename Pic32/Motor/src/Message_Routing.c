/*
 * File:   Message_Routing.c
 * Author: martin
 *
 * Created on November 17, 2013, 6:17 PM
 */
//MOTOR

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

TickType_t waitTimes[] = QOS_WAIT_TIMES;

void psForwardMessage(psMessage_t *msg, TickType_t _wait) {
    //don't change source
    TickType_t wait = _wait;
    uint8_t topicId = PS_UNDEFINED_TOPIC;
    uint8_t msgType = msg->header.messageType;

    if (msgType < PS_MSG_COUNT) {
        topicId = psDefaultTopics[msgType];
        psQOS_enum QOS = psQOS[msgType];
        if (wait > waitTimes[QOS]) wait > waitTimes[QOS];

        //fix variable length payloads
        AdjustMessageLength(msg);

        switch (topicId) {
            case LOG_TOPIC: //SysLog
                UARTProcessMessage(0, msg, wait);
                break;
            case EVENTS_TOPIC:
            case CONDITIONS_TOPIC:
                PidProcessMessage(msg, wait);
                UARTProcessMessage(0, msg, wait);
                break;
            case TICK_1S_TOPIC:
            case ANNOUNCEMENTS_TOPIC: 
                MotTaskProcessMessage(msg, wait);
                break;
            case MOT_ACTION_TOPIC:
                PidProcessMessage(msg, wait);
                break;
            default:
                UARTProcessMessage(0, msg, wait);
                break;
        }
    }
}


