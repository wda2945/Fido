/*
 * responder.c
 *
 * Responds to ping messages from the APP
 *
 *  Created on: Jul 27, 2014
 *      Author: martin
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "pubsubdata.h"
#include "helpers.h"
#include "pubsub/pubsub.h"
#include "pubsub/notifications.h"

#include "behavior/behavior.h"
#include "syslog/syslog.h"
#include "agent/agent.h"

#include "brokerq.h"
#include "broker_debug.h"

BrokerQueue_t responderQueue = BROKER_Q_INITIALIZER;

void *ResponderMessageThread(void *arg);
void sendOptionConfig(char *name, int var, int minV, int maxV, uint8_t req);
void sendSettingConfig(char *name, float var, float minV, float maxV, uint8_t req);

int configCount;
int startFlag = 1;

int ResponderInit()
{
	pthread_t thread;
	int s = pthread_create(&thread, NULL, ResponderMessageThread, NULL);
	if (s != 0)
	{
		ERRORPRINT("Responder Thread: %i\n", s);
		return -1;
	}
	return 0;
}

void ResponderProcessMessage(psMessage_t *msg)
{
	CopyMessageToQ(&responderQueue, msg);
}

//thread to receive messages and respond
void *ResponderMessageThread(void *arg)
{
	struct timespec requested_time = {0,0};
	struct timespec remaining;

	uint8_t requestor;

	DEBUGPRINT("Responder message thread started\n");

	while (1)
	{
		psMessage_t *msg = GetNextMessage(&responderQueue);

		//check message for response requirement
		switch (msg->header.messageType)
		{
		case CONFIG:
			if (msg->configPayload.responder == OVERMIND)
			{
				requestor = msg->configPayload.requestor;

				DEBUGPRINT("Send Config msg\n");
				configCount = 0;
#define optionmacro(name, var, minV, maxV, def) sendOptionConfig(name, var, minV, maxV, requestor);
#include "options.h"
#undef optionmacro


#define settingmacro(name, var, minV, maxV, def) sendSettingConfig(name, var, minV, maxV, requestor);
#include "settings.h"
#undef settingmacro

				configCount += ReportAvailableScripts();
				{
					psMessage_t msg;
					psInitPublish(msg, CONFIG_DONE);
					msg.configPayload.requestor = requestor;
					msg.configPayload.responder = OVERMIND;
					msg.configPayload.count = configCount;
					RouteMessage(&msg);
				}
				DEBUGPRINT("Config Done\n");
			}
			break;

		case PING_MSG:
		{
			if (msg->header.source != OVERMIND)
			{
				DEBUGPRINT("APP Ping msg\n");
				psMessage_t msg2;
				psInitPublish(msg2, PING_RESPONSE);
				strcpy(msg2.responsePayload.subsystem, "OVM");
				msg2.responsePayload.flags = (startFlag ? RESPONSE_FIRST_TIME : 0) |  (agentOnline ? RESPONSE_AGENT_ONLINE : 0);
				msg2.responsePayload.requestor = msg->requestPayload.requestor;
				startFlag = 0;
				RouteMessage(&msg2);

				PublishConditions(true);
				SendStats();
			}
		}
		break;
		case NEW_SETTING:
			LogInfo("Setting: %s = %f\n", msg->settingPayload.name, msg->settingPayload.value);
			DEBUGPRINT("Setting: %s = %f\n", msg->settingPayload.name, msg->settingPayload.value);
#define settingmacro(n, var, min, max, def) if (strncmp(n,msg->settingPayload.name,PS_NAME_LENGTH) == 0)\
		var = msg->settingPayload.value;
#include "settings.h"
#undef settingmacro
			break;

		case SET_OPTION:
			LogInfo("Option: %s = %i\n", msg->optionPayload.name, msg->optionPayload.value);
			DEBUGPRINT("Option: %s = %i\n", msg->optionPayload.name, msg->optionPayload.value);
#define optionmacro(n, var, min, max, def) if (strncmp(n,msg->optionPayload.name,PS_NAME_LENGTH) == 0)\
		var = msg->optionPayload.value;
#include "options.h"
#undef optionmacro
			break;
		case CONDITIONS:
		{
			int i;
			for (i=0; i<MASK_PAYLOAD_COUNT; i++)
			{
				NotificationMask_t activeSet = msg->maskPayload.value[i] & msg->maskPayload.valid[i];
				NotificationMask_t activeCancel = ~msg->maskPayload.value[i] & msg->maskPayload.valid[i];
				systemActiveConditions[i] |= activeSet;
				systemActiveConditions[i] &= ~activeCancel;
				systemValidConditions[i] |= msg->maskPayload.valid[i];
			}
		}
			break;
		default:
			//ignore anything else
			break;
		}
		DoneWithMessage(msg);
	}
	return 0;
}

void sendOptionConfig(char *name, int var, int minV, int maxV, uint8_t req) {
	struct timespec requested_time, remaining;
	psMessage_t msg;
	psInitPublish(msg, OPTION);
	strncpy(msg.optionPayload.name, name, PS_NAME_LENGTH);
	msg.optionPayload.value = var;
	msg.optionPayload.min = minV;
	msg.optionPayload.max = maxV;
	msg.optionPayload.subsystem = req;
	RouteMessage(&msg);
	configCount++;

	//delay
	requested_time.tv_sec = 0;
	requested_time.tv_nsec = MESSAGE_DELAY * 1000000;
	nanosleep(&requested_time, &remaining);
}

void sendSettingConfig(char *name, float var, float minV, float maxV, uint8_t req) {
	struct timespec requested_time, remaining;
	psMessage_t msg;
	psInitPublish(msg, SETTING);
	strncpy(msg.settingPayload.name, name, PS_NAME_LENGTH);
	msg.settingPayload.value = var;
	msg.settingPayload.min = minV;
	msg.settingPayload.max = maxV;
	msg.settingPayload.subsystem = req;
	RouteMessage(&msg);
	configCount++;

	//delay
	requested_time.tv_sec = 0;
	requested_time.tv_nsec = MESSAGE_DELAY * 1000000;
	nanosleep(&requested_time, &remaining);
}
