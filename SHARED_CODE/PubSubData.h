/* 
 * File:   PubSubData.h
 * Author: martin
 *
 * Message-related data initialized in PubSubData.c
 *
 * Created on April 19, 2014, 11:09 AM
 */

#ifndef _PUBSUBDATA_H
#define	_PUBSUBDATA_H

#include "messages.h"
#include "messages/notificationenums.h"

//Names indexed header.source
extern char *subsystemNames[SUBSYSTEM_COUNT];

//Topics by message type
extern int psDefaultTopics[PS_MSG_COUNT];

//Topic names
extern char *psTopicNames[PS_TOPIC_COUNT];

//full names
extern char *psLongMsgNames[PS_MSG_COUNT];

//Message format codes by message type
extern int psMsgFormats[PS_MSG_COUNT];

//Required QOS by message type
extern psQOS_enum psQOS[PS_MSG_COUNT];

//default length by message type (variable length messages -ve)
extern int psMessageFormatLengths[PS_FORMAT_COUNT];

//other name lookups
extern char *batteryStateNames[];
extern char *eventNames[EVENT_COUNT];
extern char *conditionNames[CONDITION_COUNT];
extern char *stateCommandNames[COMMAND_COUNT];
extern char *powerStateNames[POWER_STATE_COUNT];
extern char *ovmCommandNames[OVERMIND_ACTION_COUNT];
extern char *arbStateNames[ARB_STATE_COUNT];

#define MESSAGE_DELAY 200	//delay in ms between config messages

#endif	/* _PUBSUBDATA_H */

