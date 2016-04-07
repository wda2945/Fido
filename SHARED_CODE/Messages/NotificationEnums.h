/* 
 * File: NotificationEnum.h
 * Author: martin
 *
 * Created on April 9, 2015, 8:46 PM
 */

#ifndef NOTIFICATION_ENUMS_H
#define	NOTIFICATION_ENUMS_H

//Notifications - events

#define EVENT(e, n) e,

typedef enum {

#include "messages/notificationeventslist.h"

 EVENT_COUNT
} Event_enum;

#undef EVENT

//Notifications - conditions

#define CONDITION(e, n) e,

typedef enum {

#include "messages/notificationconditionslist.h"

    CONDITION_COUNT
} Condition_enum;

#undef CONDITION

typedef uint64_t NotificationMask_t;
#define NOTIFICATION_MASK(e) ((uint64_t)0x1 << e)

#endif	/* NOTIFICATIONS_H */

