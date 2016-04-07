/*
 * BT Callbacks.c
 *
 *  Created on: Aug 10, 2014
 *      Author: martin
 */
// BT Leaf callbacks for lua

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include <dirent.h>
#include <string.h>

#include "lua.h"
#include "lualib.h"

#include "pubsubdata.h"
#include "pubsub/pubsub.h"
#include "pubsub/notifications.h"

#include "behavior/behavior_enums.h"
#include "behavior/behavior.h"
#include "behavior/behaviordebug.h"
#include "scanner/scanner.h"
#include "syslog/syslog.h"

//actual leaf node
static int ProximityAction(lua_State *L);

typedef enum {
	isFrontLeftContact,
	isFrontContact,
	isFrontRightContact,
	isRearLeftContact,
	isRearContact,
	isRearRightContact,
	isFrontLeftProximity,
	isFrontProximity,
	isFrontRightProximity,
	isRearLeftProximity,
	isRearProximity,
	isRearRightProximity,
	isLeftProximity,
	isRightProximity,
	isFrontLeftFarProximity,
	isFrontFarProximity,
	isFrontRightFarProximity,
	isLeftPassiveProximity,
	isRightPassiveProximity,
	EnableInfrared,
	DisableInfrared,
	EnableSonar,
	DisableSonar,
	EnablePassive,
	DisablePassive,
	PROXIMITY_ACTION_COUNT
} ProximityAction_enum;

static char *proximityActionList[] = {
		"isFrontLeftContact",
		"isFrontContact",
		"isFrontRightContact",
		"isRearLeftContact",
		"isRearContact",
		"isRearRightContact",
		"isFrontLeftProximity",
		"isFrontProximity",
		"isFrontRightProximity",
		"isRearLeftProximity",
		"isRearProximity",
		"isRearRightProximity",
		"isLeftProximity",
		"isRightProximity",
		"isFrontLeftFarProximity",
		"isFrontFarProximity",
		"isFrontRightFarProximity",
		"isLeftPassiveProximity",
		"isRightPassiveProximity",
		"EnableInfrared",
		"DisableInfrared",
		"EnableSonar",
		"DisableSonar",
		"EnablePassive",
		"DisablePassive",
};

int InitProximityCallbacks(lua_State *L)
{
	int i, table;
	lua_pushcfunction(L, ProximityAction);
	lua_setglobal(L, "ProximityAction");

	lua_createtable(L, 0, PROXIMITY_ACTION_COUNT);
	table = lua_gettop(L);
	for (i=0; i< PROXIMITY_ACTION_COUNT; i++)
	{
		lua_pushstring(L, proximityActionList[i]);
		lua_pushinteger(L, i);
		lua_settable(L, table);
	}
	lua_setglobal(L, "proximity");

	return 0;
}


static int ProximityAction(lua_State *L)
{
	ProximityAction_enum actionCode 	= lua_tointeger(L, 1);
	
	lastLuaCall = proximityActionList[actionCode];	//for error reporting

	DEBUGPRINT("Proximity Action: %s\n", lastLuaCall);

	switch (actionCode)
	{
	case isFrontLeftContact:
		return actionReply(L, proximityStatus(PROX_FRONT_LEFT_MASK, PROX_CONTACT_MASK));
		break;
	case isFrontContact:
		return actionReply(L, proximityStatus(PROX_FRONT_MASK | PROX_FRONT_RIGHT_MASK | PROX_FRONT_LEFT_MASK, PROX_CONTACT_MASK));
		break;
	case isFrontRightContact:
		return actionReply(L, proximityStatus( PROX_FRONT_RIGHT_MASK, PROX_CONTACT_MASK));
		break;
	case isRearLeftContact:
		return actionReply(L, proximityStatus(PROX_REAR_LEFT_MASK, PROX_CONTACT_MASK));
		break;
	case isRearContact:
		return actionReply(L, proximityStatus(PROX_REAR_MASK | PROX_REAR_RIGHT_MASK | PROX_REAR_LEFT_MASK, PROX_CONTACT_MASK));
		break;
	case isRearRightContact:
		return actionReply(L, proximityStatus(PROX_REAR_RIGHT_MASK, PROX_CONTACT_MASK));
		break;
	case isFrontLeftProximity:
		return actionReply(L, proximityStatus(PROX_FRONT_LEFT_MASK, PROX_CONTACT_MASK | PROX_CLOSE_MASK));
		break;
	case isFrontProximity:
		return actionReply(L, proximityStatus(PROX_FRONT_MASK | PROX_FRONT_RIGHT_MASK | PROX_FRONT_LEFT_MASK, PROX_CONTACT_MASK | PROX_CLOSE_MASK));
		break;
	case isFrontRightProximity:
		return actionReply(L, proximityStatus(PROX_FRONT_RIGHT_MASK, PROX_CONTACT_MASK | PROX_CLOSE_MASK));
		break;
	case isRearLeftProximity:
		return actionReply(L, proximityStatus(PROX_REAR_LEFT_MASK, PROX_CONTACT_MASK | PROX_CLOSE_MASK));
		break;
	case isRearProximity:
		return actionReply(L, proximityStatus(PROX_REAR_MASK | PROX_REAR_RIGHT_MASK | PROX_REAR_LEFT_MASK, PROX_CONTACT_MASK | PROX_CLOSE_MASK));
		break;
	case isRearRightProximity:
		return actionReply(L, proximityStatus(PROX_REAR_RIGHT_MASK, PROX_CONTACT_MASK | PROX_CLOSE_MASK));
		break;
	case isLeftProximity:
		return actionReply(L, proximityStatus(PROX_LEFT_MASK, PROX_CLOSE_MASK));
		break;
	case isRightProximity:
		return actionReply(L, proximityStatus(PROX_RIGHT_MASK, PROX_CLOSE_MASK));
		break;
	case isFrontLeftFarProximity:
		return actionReply(L, proximityStatus(PROX_FRONT_LEFT_MASK, PROX_CONTACT_MASK | PROX_CLOSE_MASK | PROX_FAR_MASK));
		break;
	case isFrontFarProximity:
		return actionReply(L, proximityStatus(PROX_FRONT_LEFT_MASK | PROX_FRONT_RIGHT_MASK, PROX_CONTACT_MASK | PROX_CLOSE_MASK | PROX_FAR_MASK));
		break;
	case isFrontRightFarProximity:
		return actionReply(L, proximityStatus(PROX_FRONT_RIGHT_MASK, PROX_CONTACT_MASK | PROX_CLOSE_MASK | PROX_FAR_MASK));
		break;
	case isLeftPassiveProximity:
		return actionReply(L, proximityStatus(PROX_FRONT_LEFT_MASK, PROX_PASSIVE_MASK));
		break;
	case isRightPassiveProximity:
		return actionReply(L, proximityStatus(PROX_FRONT_RIGHT_MASK, PROX_PASSIVE_MASK));
		break;
	case EnableInfrared:
		return ChangeOption(L, "IR_Prox", 1);
		break;
	case DisableInfrared:
		return ChangeOption(L, "IR_Prox", 0);
		break;
	case EnableSonar:
		return ChangeOption(L, "Sonar_Prox", 1);
		break;
	case DisableSonar:
		return ChangeOption(L, "Sonar_Prox", 0);
		break;
	case EnablePassive:
		return ChangeOption(L, "PIR_Prox", 1);
		break;
	case DisablePassive:
		return ChangeOption(L, "PIR_Prox", 0);
		break;
	default:
		ERRORPRINT("Prox action: %i\n", actionCode);
		SetCondition(BT_SCRIPT_ERROR);
		return fail(L);
		break;
	}
}
