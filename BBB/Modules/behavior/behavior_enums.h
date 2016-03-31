/*
 * behavior_enums.h
 *
 *  Created on: Jul 11, 2014
 *      Author: martin
 */

#ifndef BEHAVIOR_ENUMS_H_
#define BEHAVIOR_ENUMS_H_

#include "PubSubData.h"

typedef enum {ACTION_SUCCESS, ACTION_FAIL, ACTION_RUNNING} ActionResult_enum;

extern Position_struct heelPosition;
extern bool	heelPositionValid;

extern char *lastLuaCall;
extern char *lastLuaCallFail;
extern char *lastLuaCallReason;
extern char behaviorName[PS_NAME_LENGTH];
extern BehaviorStatus_enum behaviorStatus;

typedef enum {
	PROX_FRONT_MASK = 0x1,
	PROX_FRONT_RIGHT_MASK = 0x2,
	PROX_RIGHT_MASK = 0x4,
	PROX_REAR_RIGHT_MASK = 0x8,
	PROX_REAR_MASK = 0x10,
	PROX_REAR_LEFT_MASK = 0x20,
	PROX_LEFT_MASK = 0x40,
	PROX_FRONT_LEFT_MASK = 0x80
} ProxSectorMask_enum;

typedef enum {
	PROX_OFFLINE_MASK = 0x1,
	PROX_ACTIVE_MASK = 0x2,
	PROX_ERRORS_MASK = 0x4,
	PROX_PASSIVE_MASK = 0x8,
	PROX_FAR_MASK = 0x10,
	PROX_CLOSE_MASK = 0x20,
	PROX_CONTACT_MASK = 0x40
} ProxStatusMask_enum;

#endif /* BEHAVIOR_ENUMS_H_ */
