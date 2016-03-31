/*
 * File:   PubSubData.c
 * Author: martin
 *
 * Created on April 19, 2014, 6:17 PM
 */
//Declarations for message list externs

#include "PubSubData.h"

char *subsystemNames[SUBSYSTEM_COUNT] = SUBSYSTEM_NAMES;

#define messagemacro(m,q,t,f,l) t,
int psDefaultTopics[PS_MSG_COUNT] = {
#include "Messages/MessageList.h"
};
#undef messagemacro

char *psTopicNames[PS_TOPIC_COUNT] = PS_TOPIC_NAMES;

#define messagemacro(m,q,t,f,l) l,
char *psLongMsgNames[PS_MSG_COUNT] = {
#include "Messages/MessageList.h"
};
#undef messagemacro

#define messagemacro(m,q,t,f,l) f,
int psMsgFormats[PS_MSG_COUNT] = {
#include "Messages/MessageList.h"
};
#undef messagemacro

#define messagemacro(m,q,t,f,l) q,
psQOS_enum psQOS[PS_MSG_COUNT] = {
#include "Messages/MessageList.h"
};

#undef messagemacro
#define formatmacro(e,t,v,s) s,
int psMessageFormatLengths[PS_FORMAT_COUNT] = {
#include "Messages/MsgFormatList.h"
};
#undef formatmacro

#define EVENT(e, n) n,
char *eventNames[] 	= {
#include "Messages/NotificationEventsList.h"
};
#undef EVENT

#define CONDITION(e, n) n,
char *conditionNames[]		= {
#include "Messages/NotificationConditionsList.h"
};
#undef CONDITION