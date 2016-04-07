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
#include "Helpers.h"
#include "pubsub/pubsub.h"
#include "pubsub/notifications.h"

#include "syslog/syslog.h"
#include "agent/agent.h"

#include "brokerq.h"
#include "broker_debug.h"
#include "pubsub/xbee.h"

NotificationMask_t systemActiveConditions 	= 0;
NotificationMask_t systemValidConditions	= 0;

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
	uint8_t requestor;

	DEBUGPRINT("Responder message thread started\n");

	while (1)
	{
		psMessage_t *msg = GetNextMessage(&responderQueue);

		//check message for response requirement
		switch (msg->header.messageType)
		{
		case CONFIG:
			if (msg->configPayload.responder == XBEE)
			{
				requestor = msg->configPayload.requestor;

				LogRoutine("Send Config msg\n");
				configCount = 0;
#define optionmacro(name, var, minV, maxV, def) sendOptionConfig(name, var, minV, maxV, requestor);
#include "options.h"
#undef optionmacro


#define settingmacro(name, var, minV, maxV, def) sendSettingConfig(name, var, minV, maxV, requestor);
#include "settings.h"
#undef settingmacro

				{
					psMessage_t msg;
					psInitPublish(msg, CONFIG_DONE);
					msg.configPayload.requestor = requestor;
					msg.configPayload.responder = XBEE;
					msg.configPayload.count = configCount;
					RouteMessage(&msg);
				}
				LogRoutine("Config Done\n");
			}
			break;

		case PING_MSG:
		{
			if (msg->header.source != OVERMIND)
			{
				LogRoutine("APP Ping msg\n");
				psMessage_t msg2;
				psInitPublish(msg2, PING_RESPONSE);
				strcpy(msg2.responsePayload.subsystem, "BEE");
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
			LogRoutine("Setting: %s = %f\n", msg->settingPayload.name, msg->settingPayload.value);
#define settingmacro(n, var, minV, maxV, def) if (strncmp(n,msg->settingPayload.name,PS_NAME_LENGTH) == 0)\
		var = msg->settingPayload.value;\
		sendSettingConfig(n, var, minV, maxV, 0);
#include "settings.h"
#undef settingmacro

			if (strncmp("Power Level",msg->settingPayload.name,PS_NAME_LENGTH) == 0) SetPowerLevel((int) powerLevel);

			break;

		case SET_OPTION:
			LogRoutine("Option: %s = %i\n", msg->optionPayload.name, msg->optionPayload.value);
#define optionmacro(n, var, minV, maxV, def) if (strncmp(n,msg->optionPayload.name,PS_NAME_LENGTH) == 0)\
		var = msg->optionPayload.value;\
		sendOptionConfig(n, var, minV, maxV, 0);
#include "options.h"
#undef optionmacro
			break;
		case CONDITIONS:
		{
			NotificationMask_t valid = msg->eventMaskPayload.valid;
			NotificationMask_t value = msg->eventMaskPayload.value;
			systemActiveConditions |= (valid & value);
			systemActiveConditions &= ~(valid & ~value);
			systemValidConditions |= valid;
		}
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
