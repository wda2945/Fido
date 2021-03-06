/*
 * File:   Notifications.c
 * Author: martin
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "softwareprofile.h"

#include "syslog/syslog.h"
#include "Helpers.h"
#include "pubsub.h"
#include "pubsubdata.h"
#include "notifications.h"
#include "brokerq.h"


#ifdef NOTIFICATIONS_DEBUG
#define DEBUGPRINT(...) tfprintf(stdout, __VA_ARGS__);tfprintf(psDebugFile, __VA_ARGS__);
#else
#define DEBUGPRINT(...) tfprintf(psDebugFile, __VA_ARGS__);
#endif

#define ERRORPRINT(...) tprintf(__VA_ARGS__);fprintf(psDebugFile, __VA_ARGS__);

//-------------------------------Events - triggered by changes
NotificationMask_t NotifiedEvents;

//--------------------------------Conditions - set and canceled

typedef enum {ACTIVE_MASK, VALID_MASK} Mask_enum;

NotificationMask_t active[MASK_PAYLOAD_COUNT];
NotificationMask_t valid[MASK_PAYLOAD_COUNT];
NotificationMask_t reported[MASK_PAYLOAD_COUNT];

pthread_mutex_t conditionMutex = PTHREAD_MUTEX_INITIALIZER;   //Access to ActiveConditions

NotificationMask_t systemActiveConditions[MASK_PAYLOAD_COUNT];
NotificationMask_t systemValidConditions[MASK_PAYLOAD_COUNT];

//--------------------------------Notify task and q to process events

BrokerQueue_t notifyQ = BROKER_Q_INITIALIZER;
void *EventThread(void *pvParameters);

//--------------------------------Task to publish Condition states
void *ConditionThread(void *pvParameters);

//--------------------------------

int NotificationsInit() {
	pthread_t nthread;
	NotifiedEvents = 0;

	int i;
	for (i = 0; i< MASK_PAYLOAD_COUNT; i++)
	{
		active[i] = valid[i] = reported[i] = 0;
	}

	int s = pthread_create(&nthread, NULL, ConditionThread, NULL);
	if (s != 0)
	{
		LogError("Conditions Thread: %i\n", s);
		return -1;
	}

	return 0;
}

//------------------------Events
void NotifyEvent(Event_enum e) {
	psMessage_t msg;

    if (e <= 0 || e >= EVENT_COUNT) return;

	NotificationMask_t notifyBit = NOTIFICATION_MASK(e);

	//limit repeated events to 1/sec
	//if previously notified, do not report
	if (!(NotifiedEvents & notifyBit))
	{
	    psInitPublish(msg, NOTIFY);
	    msg.intPayload.value = e;
		NewBrokerMessage(&msg);

		NotifiedEvents |= notifyBit;
	}

	DEBUGPRINT("Notify: %s\n", eventNames[e]);
}

//reset ~ 1/sec to permit a notification to be resent
void ResetNotifications() {
    NotifiedEvents = 0;
}

//--------------------Conditions
bool SetMaskBit(Condition_enum e)
{
	int bit 	= e % 64;
	int index 	= e / 64;
	NotificationMask_t mask = NOTIFICATION_MASK(bit);
	bool prior = (active[index] & valid[index] & mask);
	active[index] |= mask;
	valid[index] |= mask;
	return prior;
}

bool ClearMaskBit(Condition_enum e)
{
	int bit 	= e % 64;
	int index 	= e / 64;
	NotificationMask_t mask = NOTIFICATION_MASK(bit);
	bool prior = (active[index] & valid[index] & mask);
	active[index] &= ~mask;
	valid[index] |= mask;
	return prior;
}

bool isConditionActive(Condition_enum e)
{
	int bit 	= e % 64;
	int index 	= e / 64;

	return (active[index] & NOTIFICATION_MASK(bit));
}

void SetCondition(Condition_enum e)
{
    if (e <= 0 || e >= CONDITION_COUNT) return;
    bool prior;
    pthread_mutex_lock(&conditionMutex);
    prior = SetMaskBit(e);
    pthread_mutex_unlock(&conditionMutex);

    if (!prior) DEBUGPRINT("conditions: Set: %s\n", conditionNames[e]);
}

void CancelCondition(Condition_enum e)
{
    if (e <= 0 || e >= CONDITION_COUNT) return;
    bool prior;
    
    pthread_mutex_lock(&conditionMutex);
    prior = ClearMaskBit(e);
    pthread_mutex_unlock(&conditionMutex);

    if (prior) DEBUGPRINT("conditions: Cancel: %s\n", conditionNames[e]);
}

//publish conditions if changed, or if forced
void PublishConditions(bool _force) {
    psMessage_t msg;
    bool sendit = _force;
    
    pthread_mutex_lock(&conditionMutex);

    if (!_force)
    {
    	int i;
    	for (i = 0; i< MASK_PAYLOAD_COUNT; i++)
    	{
    		if (active[i] != reported[i])
    		{
    			sendit = true;
    			break;
    		}
    	}
    }

    if (sendit)
    {  
        psInitPublish(msg, CONDITIONS);

    	int i;

    	for (i = 0; i< MASK_PAYLOAD_COUNT; i++)
    	{
    	    msg.maskPayload.value[i] = active[i];
    	    msg.maskPayload.valid[i] = valid[i];
    		reported[i] = active[i];
    	}
        RouteMessage(&msg);
    }
    pthread_mutex_unlock(&conditionMutex);
}

//sends a conditions update every 250ms, if needed
#define PUBLISH_INTERVAL 	250
#define RESET_INTERVAL		1000
void *ConditionThread(void *pvParameters)
{
	struct timespec request, remain;
	int loopCount = RESET_INTERVAL / PUBLISH_INTERVAL;
	while(1)
	{
		PublishConditions(false);

		//sleep 250ms
		request.tv_sec = 0;
		request.tv_nsec = PUBLISH_INTERVAL * 1000000;	//250 millisecs
		nanosleep(&request, &remain);

		if (loopCount-- <= 0)
		{
			ResetNotifications();
			loopCount = RESET_INTERVAL / PUBLISH_INTERVAL;
		}
	}
}
