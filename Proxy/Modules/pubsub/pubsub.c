/*
 ============================================================================
gd Name        : Broker.c
 Author      : Martin
 Version     :
 Copyright   : (c) 2013 Martin Lane-Smith
 Description : Linux PubSub broker
 ============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "pubsubdata.h"
#include "Helpers.h"
#include "pubsub/pubsub.h"

#include "syslog/syslog.h"
#include "agent/agent.h"
#include "softwareprofile.h"

#include "brokerq.h"
#include "broker_debug.h"

FILE *psDebugFile;

bool PubSubBrokerReady = false;

//broker structures
//input queue
BrokerQueue_t brokerQueue = BROKER_Q_INITIALIZER;

void *BrokerInputThread(void *args);

int PubSubInit()
{
	pthread_t inputThread;

    {
        char buff[100];
        sprintf(buff, "%s%s", LOGFILE_FOLDER, "broker.log");
        
        psDebugFile = fopen(buff, "w");
    }

	//create thread to receive messages from the broker queue
	int s = pthread_create(&inputThread, NULL, BrokerInputThread, NULL);
	if (s != 0)
	{
		ERRORPRINT("broker: Input Thread error - %i", s);
		return -1;
	}
	return 0;
}

//processes messages from the broker queue
void *BrokerInputThread(void *args)
{
	PubSubBrokerReady = true;

	while(1)
	{
		//process messages off the broker queue
		psMessage_t *msg = GetNextMessage(&brokerQueue);

//		if (msg->header.messageType != SYSLOG_MSG && msg->header.messageType != TICK_1S &&
//				msg->header.messageType != BBBLOG_MSG)
//			DEBUGPRINT("broker: %s\n", psLongMsgNames[msg->header.messageType]);

		RouteMessage(msg);

		DoneWithMessage(msg);
	}
}

//pass message to appropriate modules
void RouteMessage(psMessage_t *msg)
{

	if (!PubSubBrokerReady) return;

    uint8_t topicId = msg->header.messageTopic;
    uint8_t msgType = msg->header.messageType;

    if (msgType < PS_MSG_COUNT)
    {
    	msg->header.messageTopic = topicId = psDefaultTopics[msgType];
        //fix variable length payloads
        AdjustMessageLength(msg);
    }

	switch(topicId)
	{
	default:
		XBeeBrokerProcessMessage(msg);
		break;
	case PS_UNDEFINED_TOPIC:
        break;
	case LOG_TOPIC:              //SysLog
		LogProcessMessage(msg);
		AgentProcessMessage(msg);
		break;
	case OVM_LOG_TOPIC:
		LogProcessMessage(msg);
		break;
	case ANNOUNCEMENTS_TOPIC:    //Common channel
		ResponderProcessMessage(msg);
		XBeeBrokerProcessMessage(msg);
		AgentProcessMessage(msg);
		break;
	case EVENTS_TOPIC:
		XBeeBrokerProcessMessage(msg);
		ResponderProcessMessage(msg);
		LogProcessMessage(msg);
		break;
	case CONDITIONS_TOPIC:
		XBeeBrokerProcessMessage(msg);
		AgentProcessMessage(msg);
		ResponderProcessMessage(msg);
		LogProcessMessage(msg);
		break;
	case APP_REPORT_TOPIC:       //To send data to APP
	case SYS_REPORT_TOPIC:       //Reports to APP & OVM
	case RESPONSE_TOPIC:
		AgentProcessMessage(msg);
		break;
	case MCP_ACTION_TOPIC:
	case MOT_ACTION_TOPIC:
	case OVM_ACTION_TOPIC:
		XBeeBrokerProcessMessage(msg);
		break;
	}
}

