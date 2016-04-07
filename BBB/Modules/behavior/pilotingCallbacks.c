/*
 * pilotingCallbacks.c
 *
 *  Created on: Aug 10, 2014
 *      Author: martin
 */
// BT Leaf LUA callbacks for piloting

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
#include "autopilot/autopilot.h"
#include "navigator/navigator.h"
#include "syslog/syslog.h"

//actual leaf node
static int PilotAction(lua_State *L);

typedef enum {
	isGoodPose,
	isAtWaypoint,
	WaitForFix,
	isPilotReady,
	ComputeHomePosition,
	ComputeSolarPosition,
	ComputeHeelPosition,
	ComputeRandomExplorePosition,
	ComputeRandomClosePosition,
	Orient,
	Engage,
	Turn,
	TurnLeft,
	TurnRight,
	TurnN,
	TurnS,
	TurnE,
	TurnW,
	TurnLeft90,
	TurnRight90,
	MoveForward,
	MoveBackward,
	MoveForward10,
	MoveBackward10,
	SetFastSpeed,
	SetMediumSpeed,
	SetLowSpeed,
	EnableFrontContactStop,
	DisableFrontContactStop,
	EnableFrontCloseStop,
	DisableFrontCloseStop,
	EnableRearContactStop,
	DisableRearContactStop,
	EnableRearCloseStop,
	DisableRearCloseStop,
	EnableFrontFarStop,
	DisableFrontFarStop,
	EnableWaypointStop,
	DisableWaypointStop,
	EnableLostFixStop,
	DisableLostFixStop,
	PILOTING_ACTION_COUNT
} PilotingAction_enum;

static char *pilotingActionList[] = {
		"isGoodPose",
		"isAtWaypoint",
		"WaitForFix",
		"isPilotReady",
		"ComputeHomePosition",
		"ComputeSolarPosition",
		"ComputeHeelPosition",
		"ComputeRandomExplorePosition",
		"ComputeRandomClosePosition",
		"Orient",
		"Engage",
		"Turn",
		"TurnLeft",
		"TurnRight",
		"TurnN",
		"TurnS",
		"TurnE",
		"TurnW",
		"TurnLeft90",
		"TurnRight90",
		"MoveForward",
		"MoveBackward",
		"MoveForward10",
		"MoveBackward10",
		"SetFastSpeed",
		"SetMediumSpeed",
		"SetLowSpeed",
		"EnableFrontContactStop",
		"DisableFrontContactStop",
		"EnableFrontCloseStop",
		"DisableFrontCloseStop",
		"EnableRearContactStop",
		"DisableRearContactStop",
		"EnableRearCloseStop",
		"DisableRearCloseStop",
		"EnableFrontFarStop",
		"DisableFrontFarStop",
		"EnableWaypointStop",
		"DisableWaypointStop",
		"EnableLostFixStop",
		"DisableLostFixStop",
};

Position_struct heelPosition;
bool	heelPositionValid = false;

int InitPilotingCallbacks(lua_State *L)
{
	int i, table;
	lua_pushcfunction(L, PilotAction);
	lua_setglobal(L, "PilotAction");

	lua_createtable(L, 0, PILOTING_ACTION_COUNT);
	table = lua_gettop(L);
	for (i=0; i< PILOTING_ACTION_COUNT; i++)
	{
		lua_pushstring(L, pilotingActionList[i]);
		lua_pushinteger(L, i);
		lua_settable(L, table);
	}
	lua_setglobal(L, "pilot");

	return 0;
}

//callback from lua BT Leaf
static int PilotAction(lua_State *L)
{
	PilotingAction_enum actionCode 	= lua_tointeger(L, 1);

	lastLuaCall = pilotingActionList[actionCode];	//for error reporting

	DEBUGPRINT("Pilot Action: %s\n", lastLuaCall);

	switch (actionCode)
	{
	case isGoodPose:
		return actionReply(L, NavigatorIsGoodPose(false));
		break;
	case isAtWaypoint:
		return actionReply(L, pilotIsAtWaypoint());
		break;
	case WaitForFix:
		return success(L);//actionReply(L, NavigatorIsGoodPose(true));
		break;
	case isPilotReady:
		return actionReply(L, AutopilotIsReadyToMove());
		break;
	case ComputeHomePosition:
		return actionReply(L, pilotSetGoalWaypoint("Home"));
		break;
	case ComputeSolarPosition:
		return actionReply(L, pilotSetGoalWaypoint("Solar1"));
		break;
	case ComputeHeelPosition:
		if (heelPositionValid)
		{
			return actionReply(L, pilotSetGoalPosition(heelPosition));
		}
		else
		{
			return fail(L);
		}
		break;
	case ComputeRandomExplorePosition:
		return actionReply(L, pilotSetRandomGoal(500)); 		//1 meter
		break;
	case ComputeRandomClosePosition:
		return actionReply(L, pilotSetRandomGoal(30)); 		//1 foot
		break;
	case Orient:
		return actionReply(L, AutopilotAction(PILOT_ORIENT));
		break;
	case Engage:
		return actionReply(L, AutopilotAction(PILOT_ENGAGE));
		break;
	case Turn:
		if (drand48() > 0.5)
			return actionReply(L, AutopilotAction(PILOT_TURN_LEFT));
		else
			return actionReply(L, AutopilotAction(PILOT_TURN_RIGHT));
		break;
	case TurnLeft:
		return actionReply(L, AutopilotAction(PILOT_TURN_LEFT));
		break;
	case TurnRight:
		return actionReply(L, AutopilotAction(PILOT_TURN_RIGHT));
		break;
	case TurnN:
		return actionReply(L, AutopilotAction(PILOT_TURN_N));
		break;
	case TurnS:
		return actionReply(L, AutopilotAction(PILOT_TURN_S));
		break;
	case TurnE:
		return actionReply(L, AutopilotAction(PILOT_TURN_E));
		break;
	case TurnW:
		return actionReply(L, AutopilotAction(PILOT_TURN_W));
		break;
	case TurnLeft90:
		return actionReply(L, AutopilotAction(PILOT_TURN_LEFT_90));
		break;
	case TurnRight90:
		return actionReply(L, AutopilotAction(PILOT_TURN_RIGHT_90));
		break;
	case MoveForward:
		return actionReply(L, AutopilotAction(PILOT_MOVE_FORWARD));
		break;
	case MoveBackward:
		return actionReply(L, AutopilotAction(PILOT_MOVE_BACKWARD));
		break;
	case MoveForward10:
		return actionReply(L, AutopilotAction(PILOT_MOVE_FORWARD_10));
		break;
	case MoveBackward10:
		return actionReply(L, AutopilotAction(PILOT_MOVE_BACKWARD_10));
		break;
	case SetFastSpeed:
		return actionReply(L, AutopilotAction(PILOT_MOVE_BACKWARD));
		break;
	case SetMediumSpeed:
		return actionReply(L, AutopilotAction(PILOT_MOVE_BACKWARD));
		break;
	case SetLowSpeed:
		return actionReply(L, AutopilotAction(PILOT_MOVE_BACKWARD));
		break;
	case EnableFrontContactStop:
		pilotFlags |= ENABLE_FRONT_CONTACT_ABORT;
		return success(L);
		break;
	case DisableFrontContactStop:
		pilotFlags &= ~ENABLE_FRONT_CONTACT_ABORT;
		return success(L);
		break;
	case EnableFrontCloseStop:
		pilotFlags |= ENABLE_FRONT_CLOSE_ABORT;
		return success(L);
		break;
	case DisableFrontCloseStop:
		pilotFlags &= ~ENABLE_FRONT_CLOSE_ABORT;
		return success(L);
		break;
	case EnableRearContactStop:
		pilotFlags |= ENABLE_REAR_CONTACT_ABORT;
		return success(L);
		break;
	case DisableRearContactStop:
		pilotFlags &= ~ENABLE_REAR_CONTACT_ABORT;
		return success(L);
		break;
	case EnableRearCloseStop:
		pilotFlags |= ENABLE_REAR_CLOSE_ABORT;
		return success(L);
		break;
	case DisableRearCloseStop:
		pilotFlags &= ~ENABLE_REAR_CLOSE_ABORT;
		return success(L);
		break;
	case EnableFrontFarStop:
		pilotFlags |= ENABLE_FRONT_FAR_ABORT;
		return success(L);
		break;
	case DisableFrontFarStop:
		pilotFlags &= ~ENABLE_FRONT_FAR_ABORT;
		return success(L);
		break;
	case EnableWaypointStop:
		pilotFlags |= ENABLE_WAYPOINT_STOP;
		return success(L);
		break;
	case DisableWaypointStop:
		pilotFlags &= ~ENABLE_WAYPOINT_STOP;
		return success(L);
		break;
	case EnableLostFixStop:
		pilotFlags |= ENABLE_LOSTFIX_ABORT;
		return success(L);
		break;
	case DisableLostFixStop:
		pilotFlags &= ~ENABLE_LOSTFIX_ABORT;
		return success(L);
		break;
	default:
		ERRORPRINT("Nav action: %i\n", actionCode);
		SetCondition(BT_SCRIPT_ERROR);
		return fail(L);
		break;
	}

}
