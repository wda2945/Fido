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

#include "PubSubData.h"
#include "Helpers.h"
#include "pubsub/pubsub.h"

#include "behavior/behavior.h"
#include "navigator/navigator.h"
#include "scanner/scanner.h"
#include "autopilot/autopilot.h"
#include "syslog/syslog.h"
#include "agent/agent.h"
#include "SoftwareProfile.h"

#include "brokerQ.h"
#include "broker_debug.h"

FILE *psDebugFile;

bool PubSubBrokerReady = false;

//broker structures
//input queue
BrokerQueue_t brokerQueue = BROKER_Q_INITIALIZER;

void *BrokerInputThread(void *args);

int PubSubInit()
{
	int reply;
	pthread_t inputThread;

	psDebugFile = fopen("/root/logfiles/broker.log", "w");

	//create thread to receive messages from the broker queue
	int s = pthread_create(&inputThread, NULL, BrokerInputThread, NULL);
	if (s != 0)
	{
		ERRORPRINT("Input Thread: %n", s);
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

		if (msg->header.messageType != SYSLOG_MSG && msg->header.messageType != TICK_1S &&
				msg->header.messageType != BBBLOG_MSG)
			DEBUGPRINT("Broker: %s\n", psLongMsgNames[msg->header.messageType]);

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

//      DebugPrint("Forward: %s", psLongMsgNames[msgType]);
    }
    else
    {
//    DebugPrint("Forward: %i in %s", msgType, psTopicNames[topicId]);
    }

	switch(topicId)
	{
	default:
		break;
	case LOG_TOPIC:              //SysLog
		LogProcessMessage(msg);
		SerialBrokerProcessMessage(msg);
		AgentProcessMessage(msg);
		SerialBrokerProcessMessage(msg);
		break;
	case OVM_LOG_TOPIC:
		LogProcessMessage(msg);
		break;
	case ANNOUNCEMENTS_TOPIC:    //Common channel
		BehaviorProcessMessage(msg);
		SerialBrokerProcessMessage(msg);
		AgentProcessMessage(msg);
		ResponderProcessMessage(msg);
		break;
	case RESPONSE_TOPIC:
		BehaviorProcessMessage(msg);
		AgentProcessMessage(msg);
		SerialBrokerProcessMessage(msg);
		break;
	case EVENTS_TOPIC:
		SerialBrokerProcessMessage(msg);
		BehaviorProcessMessage(msg);
		AutopilotProcessMessage(msg);
		ResponderProcessMessage(msg);
		LogProcessMessage(msg);
		break;
	case CONDITIONS_TOPIC:
		SerialBrokerProcessMessage(msg);
		AgentProcessMessage(msg);
		BehaviorProcessMessage(msg);
		AutopilotProcessMessage(msg);
		ResponderProcessMessage(msg);
		LogProcessMessage(msg);
		break;
	case APP_REPORT_TOPIC:       //To send data to APP
		AgentProcessMessage(msg);
		SerialBrokerProcessMessage(msg);
		break;
	case MOT_ACTION_TOPIC:		 //To send commands to the motors
		SerialBrokerProcessMessage(msg);
		break;
	case NAV_REPORT_TOPIC:       //Fused Odometry & Prox Reports
		AutopilotProcessMessage(msg);
		BehaviorProcessMessage(msg);
		break;
	case OVM_ACTION_TOPIC:
		BehaviorProcessMessage(msg);
		break;
	case SYS_REPORT_TOPIC:       //Reports to APP & OVM
		ResponderProcessMessage(msg);
		BehaviorProcessMessage(msg);
		AgentProcessMessage(msg);
		break;
	case RAW_NAV_TOPIC:          //Raw compass & GPS
		NavigatorProcessMessage(msg);
		break;
	case RAW_ODO_TOPIC:          //Raw Odometry
		AutopilotProcessMessage(msg);
		NavigatorProcessMessage(msg);
		break;
	case RAW_PROX_TOPIC:	    //Raw Prox reports
		ScannerProcessMessage(msg);
		break;
	case TICK_1S_TOPIC:          //1 second ticks
		AutopilotProcessMessage(msg);
		NavigatorProcessMessage(msg);
		BehaviorProcessMessage(msg);
		break;
	}
}

