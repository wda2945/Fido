/*
 * File:   Routing.c
 * Author: martin
 *
 * Created on November 17, 2013, 6:17 PM
 */

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSubData.h"
#include "SoftwareProfile.h"

#include "FreeRTOS.h"

//non-PIC messages

TickType_t waitTimes[] = QOS_WAIT_TIMES;

void psForwardMessage(psMessage_t *msg, TickType_t _wait) {
    //don't change source
    TickType_t wait = _wait;

    uint8_t topicId = msg->header.messageTopic;
    uint8_t msgType = msg->header.messageType;

    if (msgType < PS_MSG_COUNT)
    {
        topicId = psDefaultTopics[msgType];
        //fix variable length payloads
        AdjustMessageLength(msg);
    
        psQOS_enum QOS = psQOS[msgType];
        if (wait > waitTimes[QOS]) wait = waitTimes[QOS];

    }
     
    switch (topicId) {
        case ANNOUNCEMENTS_TOPIC:   //Common channel
        default:                    //unknown topic
            McpTaskProcessMessage(msg, wait);
            BLEProcessMessage(msg, wait);
            UARTProcessMessage(0, msg, wait);
            XBeeBrokerProcessMessage(msg, wait);
                    break;
        case CONDITIONS_TOPIC:
            BLEProcessMessage(msg, wait);
            UARTProcessMessage(0, msg, wait);
            XBeeBrokerProcessMessage(msg, wait);
            break;
        case EVENTS_TOPIC:
            UARTProcessMessage(0, msg, wait);
            McpTaskProcessMessage(msg, wait);
            break;
        case MCP_ACTION_TOPIC:       //Move commands
            McpTaskProcessMessage(msg, wait);
            break;
        case MOT_ACTION_TOPIC:       //Move commands
            UARTProcessMessage(MOTOR_SUBSYSTEM, msg, wait);
            break;
        case OVM_ACTION_TOPIC:
        case RAW_ODO_TOPIC:
        case RAW_NAV_TOPIC:
        case RAW_PROX_TOPIC:
            UARTProcessMessage(OVERMIND, msg, wait);
            break;
        case APP_REPORT_TOPIC:           //To send data to APP
            UARTProcessMessage(OVERMIND, msg, wait);
            BLEProcessMessage(msg, wait);
            XBeeBrokerProcessMessage(msg, wait);
            break;
        case LOG_TOPIC:
        case SYS_REPORT_TOPIC:
        case RESPONSE_TOPIC:
            BLEProcessMessage(msg, wait);
            UARTProcessMessage(OVERMIND, msg, wait);
            XBeeBrokerProcessMessage(msg, wait);
            break;
        case TICK_1S_TOPIC:
            UARTProcessMessage(0, msg, wait);
    break;
    }

}


