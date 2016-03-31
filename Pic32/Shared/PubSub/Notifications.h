/* 
 * File:   Notifications.h
 * Author: martin
 *
 * Created on April 11, 2015, 9:21 PM
 */

#ifndef NOTIFICATIONS_H
#define	NOTIFICATIONS_H

//NOTIFICATIONS

int NotificationsInit();

//Raise a event (listed in NotificationsEnums.h
void NotifyEvent(Event_enum e);
TickType_t EventLastNotified(Event_enum e);

//Set a condition
void SetCondition(Condition_enum e);
void CancelCondition(Condition_enum e);
#define Condition(e, b) {if (b) SetCondition(e); else CancelCondition(e);}

//test state
bool isConditionActive(Condition_enum e);
NotificationMask_t GetActiveConditions();
TickType_t GetConditionLastSetTime(Condition_enum e);
TickType_t GetConditionLastCancelTime(Condition_enum e);

#endif	/* NOTIFICATIONS_H */

