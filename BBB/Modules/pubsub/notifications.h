/* 
 * File:   Notifications.h
 * Author: martin
 *
 * Created on April 11, 2015, 9:21 PM
 */

#ifndef NOTIFICATIONS_H
#define	NOTIFICATIONS_H

#include "PubSubData.h"

//NOTIFICATIONS

//SYSTEM-WIDE (maintained by responder)
extern NotificationMask_t systemActiveConditions[MASK_PAYLOAD_COUNT];
extern NotificationMask_t systemValidConditions[MASK_PAYLOAD_COUNT];

bool isConditionActive(Condition_enum e);

extern char *eventNames[];
extern char *conditionNames[];

int NotificationsInit();

//Raise a event (listed in NotificationsEnums.h
void NotifyEvent(Event_enum e);

//Set a condition
void SetCondition(Condition_enum e);
void CancelCondition(Condition_enum e);
#define Condition(e, b) {if (b) SetCondition(e); else CancelCondition(e);}

void PublishConditions(bool force) ;
void ResetNotifications();  //reset throttling limits - called periodically (~1/sec?)

#endif	/* NOTIFICATIONS_H */

