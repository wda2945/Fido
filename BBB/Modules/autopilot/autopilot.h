/*
 * autopilot.h
 *
 *  Created on: 2015
 *      Author: martin
 */


#ifndef AUTOPILOT_H_
#define AUTOPILOT_H_

#include "behavior/behavior_enums.h"

//init autopilot task
int AutopilotInit();
//messages
void AutopilotProcessMessage(psMessage_t *msg);

#define PILOT_LOOP_MS 250;

extern uint16_t pilotFlags;

//autopilot api

ActionResult_enum pilotSetGoalWaypoint(char *waypointName);
ActionResult_enum pilotSetGoalPosition(Position_struct _goal);
ActionResult_enum pilotSetRelativeGoal(int _rangeCM, int _bearingDegrees);
ActionResult_enum pilotSetRandomGoal(int _rangeCM);
ActionResult_enum pilotIsAtWaypoint();

typedef enum {
	PILOT_RESET,
	PILOT_ORIENT,
	PILOT_ENGAGE,
	PILOT_TURN_LEFT,
	PILOT_TURN_RIGHT,
	PILOT_TURN_LEFT_90,
	PILOT_TURN_RIGHT_90,
	PILOT_TURN_N,
	PILOT_TURN_S,
	PILOT_TURN_E,
	PILOT_TURN_W,
	PILOT_MOVE_FORWARD,
	PILOT_MOVE_BACKWARD,
	PILOT_MOVE_FORWARD_10,
	PILOT_MOVE_BACKWARD_10,
	PILOT_FAST_SPEED,
	PILOT_MEDIUM_SPEED,
	PILOT_SLOW_SPEED
} PilotAction_enum;

#define PILOT_ACTION_NAMES {\
		"Pilot_Reset",\
		"Pilot_Orient",\
		"Pilot_Engage",\
		"Pilot_Turn_Left",\
		"Pilot_Turn_Right",\
		"Pilot_Turn_Left_90",\
		"Pilot_Turn_Right_90",\
		"Pilot_Turn_N",\
		"Pilot_Turn_S",\
		"Pilot_Turn_E",\
		"Pilot_Turn_W",\
		"Pilot_Move_Forward",\
		"Pilot_Move_Backward",\
		"Pilot_Move_Forward_10",\
		"Pilot_Move_Backward_10",\
		"Pilot_Fast_Speed",\
		"Pilot_Medium_Speed",\
		"Pilot_Slow_Speed"\
		}

extern char *pilotActionNames[];

ActionResult_enum AutopilotAction(PilotAction_enum _action);

ActionResult_enum AutopilotIsReadyToMove();

#endif
