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
extern NotificationMask_t systemActiveConditions;
extern NotificationMask_t systemValidConditions;
#define isConditionActive(e) (NOTIFICATION_MASK((e)) & systemActiveConditions & systemValidConditions)

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

