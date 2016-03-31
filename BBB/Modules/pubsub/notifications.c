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

#include "SoftwareProfile.h"

#include "sysLog/syslog.h"
#include "Helpers.h"
#include "pubsub.h"
#include "PubSubData.h"
#include "notifications.h"
#include "brokerQ.h"


#ifdef NOTIFICATIONS_DEBUG
#define DEBUGPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(psDebugFile, __VA_ARGS__);fflush(psDebugFile);
#else
#define DEBUGPRINT(...) fprintf(psDebugFile, __VA_ARGS__);fflush(psDebugFile);
#endif

#define ERRORPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(psDebugFile, __VA_ARGS__);fflush(psDebugFile);

//-------------------------------Events - triggered by changes
NotificationMask_t NotifiedEvents;

//--------------------------------Conditions - set and canceled

NotificationMask_t ActiveConditions;
NotificationMask_t ReportedConditions;
NotificationMask_t ValidConditions;

pthread_mutex_t conditionMutex = PTHREAD_MUTEX_INITIALIZER;   //Access to ActiveConditions

//--------------------------------Notify task and q to process events

BrokerQueue_t notifyQ = BROKER_Q_INITIALIZER;
void *EventThread(void *pvParameters);

//--------------------------------Task to publish Condition states
void *ConditionThread(void *pvParameters);

//--------------------------------

int NotificationsInit() {
	int i;
	pthread_t nthread;
	NotifiedEvents = ValidConditions = ReportedConditions = ActiveConditions = 0;

	int s = pthread_create(&nthread, NULL, ConditionThread, NULL);
	if (s != 0)
	{
		ERRORPRINT("onditions Thread: %i\n", s);
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

	LogRoutine("Notify: %s\n", eventNames[e]);
}

//reset ~ 1/sec to permit a notification to be resent
void ResetNotifications() {
    NotifiedEvents = 0;
}

//--------------------Conditions
void SetCondition(Condition_enum e)
{
    if (e <= 0 || e >= CONDITION_COUNT) return;
    NotificationMask_t maskBit = NOTIFICATION_MASK(e);
    
    pthread_mutex_lock(&conditionMutex);
    ActiveConditions |= maskBit;
    ValidConditions |= maskBit;
    pthread_mutex_unlock(&conditionMutex);

    LogRoutine("Set: %s\n", conditionNames[e]);
}

void CancelCondition(Condition_enum e)
{
    if (e <= 0 || e >= CONDITION_COUNT) return;
    NotificationMask_t maskBit = NOTIFICATION_MASK(e);
    
    pthread_mutex_lock(&conditionMutex);
    ActiveConditions &= ~maskBit;
    ValidConditions |= maskBit;
    pthread_mutex_unlock(&conditionMutex);

    LogRoutine("Cancel: %s\n", conditionNames[e]);
}

//publish conditions if changed, or if forced
void PublishConditions(bool force) {
    psMessage_t msg;
    
    if ((ReportedConditions != ActiveConditions) || force)
    {  
        psInitPublish(msg, CONDITIONS);
        pthread_mutex_lock(&conditionMutex);
        msg.eventMaskPayload.value = ReportedConditions = ActiveConditions;
        msg.eventMaskPayload.valid = ValidConditions;
        pthread_mutex_unlock(&conditionMutex);

        RouteMessage(&msg);
    }
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
