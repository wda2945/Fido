/*
 ============================================================================
 Name        : autopilot.c
 Author      : Martin
 Version     :
 Copyright   : (c) 2015 Martin Lane-Smith
 Description : Receives MOVE commands and issues commands to the motors, comparing
 navigation results against the goal
 ============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stropts.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>

#include "softwareprofile.h"
#include "pubsubdata.h"
#include "pubsub/pubsub.h"
#include "pubsub/notifications.h"

#include "syslog/syslog.h"

#include "navigator/navigator.h"
#include "autopilot.h"
#include "autopilot_common.h"
#include "planner.h"
#include "behavior/behavior.h"
#include "waypoints.h"

FILE *pilotDebugFile;

BrokerQueue_t autopilotQueue = BROKER_Q_INITIALIZER;

//Autopilot thread
void *AutopilotThread(void *arg);
pthread_mutex_t	pilotMtx = PTHREAD_MUTEX_INITIALIZER;

bool VerifyCompass();
bool VerifyGPS();

int ComputeBearing(Position_struct _position);

//for new movement commands
bool ProcessActionCommand(PilotAction_enum _action);
char *pilotActionNames[] = PILOT_ACTION_NAMES;

//start/continue action
bool PerformOrient();
bool PerformMovement();
//send motor command
void SendMotorCommand(int _port, int _starboard, float _speed, uint16_t _flags);
void ResendMotorCommand();

#define RANDOM_MOVE_SPREAD 20	//cm
int CalculateRandomDistance();
#define RANDOM_TURN_SPREAD 30	//degrees
int CalculateRandomTurn();

//autopilot state machine
typedef enum {
	PILOT_STATE_IDLE,			//ready for a command
	PILOT_STATE_FORWARD_SENT,	//short distance move using encoders only
	PILOT_STATE_BACKWARD_SENT,	//ditto
	PILOT_STATE_ORIENT_SENT,	//monitoring motion using compass
	PILOT_STATE_MOVE_SENT,		//monitoring move using pose msg
	PILOT_STATE_FORWARD,		//short distance move using encoders only
	PILOT_STATE_BACKWARD,		//ditto
	PILOT_STATE_ORIENT,			//monitoring motion using compass
	PILOT_STATE_MOVE,			//monitoring move using pose msg
	PILOT_STATE_DONE,			//move complete
	PILOT_STATE_FAILED,			//move failed
	PILOT_STATE_ABORT,
	PILOT_STATE_INACTIVE		//motors disabled
} PilotState_enum;

static char *pilotStateNames[] = {
		"IDLE",					//ready for a command
		"FORWARD_SENT",			//short distance move using encoders only
		"BACKWARD_SENT",		//ditto
		"ORIENT_SENT",			//monitoring motion using compass
		"MOVE_SENT",			//monitoring move using pose msg
		"FORWARD",				//short distance move using encoders only
		"BACKWARD",				//ditto
		"ORIENT",				//monitoring motion using compass
		"MOVE",					//monitoring move using pose msg
		"DONE",					//move complete
		"FAILED",				//move failed
		"ABORT",
		"INACTIVE"				//motors disabled
};

PilotState_enum pilotState = PILOT_STATE_INACTIVE;
void ChangePilotState(PilotState_enum newState);
pthread_mutex_t	pilotStateMtx = PTHREAD_MUTEX_INITIALIZER;

bool motorsInhibit 	= true;
bool motorsBusy 	= false;
bool motorsErrors 	= false;

NotificationMask_t currentConditions;

NotificationMask_t frontCloseMask = NOTIFICATION_MASK(FRONT_LEFT_PROXIMITY)
		| NOTIFICATION_MASK(FRONT_CENTER_PROXIMITY)
		| NOTIFICATION_MASK(FRONT_RIGHT_PROXIMITY);
NotificationMask_t rearCloseMask = NOTIFICATION_MASK(REAR_LEFT_PROXIMITY)
		| NOTIFICATION_MASK(REAR_CENTER_PROXIMITY)
		| NOTIFICATION_MASK(REAR_RIGHT_PROXIMITY);
NotificationMask_t frontFarMask = NOTIFICATION_MASK(FRONT_LEFT_FAR_PROXIMITY)
		| NOTIFICATION_MASK(FRONT_RIGHT_FAR_PROXIMITY);

time_t MOVE_XXX_SENT_time = 0;
time_t MOVE_XXX_time = 0;
int motorsRunTimeout;

uint16_t pilotFlags = 0;		//MotorFlags_enum
float motorSpeed;

#define CM_PER_DEGREE (60.0 * 5280.0 * 12.0 * 2.54)	//convert lat-long difference to cm

int AutopilotInit() {

	pilotDebugFile = fopen("/root/logfiles/pilot.log", "w");

	//load waypoints
	LoadWaypointDatabase(WAYPOINT_FILE_PATH);

	if (InitNodeList() < 0)
	{
		ERRORPRINT("pilot: InitNodeList fail");
		return -1;
	}

	//create autopilot thread
	pthread_t thread;
	int s = pthread_create(&thread, NULL, AutopilotThread, NULL);
	if (s != 0) {
		ERRORPRINT("pilot: Thread fail %i", s);
		return -1;
	}

	return 0;
}

void ChangePilotState(PilotState_enum newState)
{
	//critical section
	int s = pthread_mutex_lock(&pilotStateMtx);
	if (s != 0)
	{
		ERRORPRINT("pilot: pilotStateMtx lock %i\n", s);
	}

	//send update notifications
	if (newState != pilotState) {
		DEBUGPRINT("pilot: New State: %s\n", pilotStateNames[newState]);

		//the state we're leaving
		switch (pilotState) {
		case PILOT_STATE_IDLE:
		case PILOT_STATE_INACTIVE:
			CancelCondition(PILOT_IDLE);
			break;
		case PILOT_STATE_FORWARD_SENT:
		case PILOT_STATE_BACKWARD_SENT:
		case PILOT_STATE_ORIENT_SENT:
		case PILOT_STATE_MOVE_SENT:
		case PILOT_STATE_FORWARD:
		case PILOT_STATE_BACKWARD:
		case PILOT_STATE_ORIENT:
		case PILOT_STATE_MOVE:
			CancelCondition(PILOT_ENGAGED);
			break;
		case PILOT_STATE_DONE:
			CancelCondition(PILOT_IDLE);
			break;
		case PILOT_STATE_FAILED:
		case PILOT_STATE_ABORT:
			CancelCondition(PILOT_FAILED);
			break;
		}

		//the new state
		switch (newState) {
		case PILOT_STATE_IDLE:
		case PILOT_STATE_INACTIVE:
			SetCondition(PILOT_IDLE);
			break;
		case PILOT_STATE_FORWARD_SENT:
		case PILOT_STATE_BACKWARD_SENT:
		case PILOT_STATE_ORIENT_SENT:
		case PILOT_STATE_MOVE_SENT:
		case PILOT_STATE_FORWARD:
		case PILOT_STATE_BACKWARD:
		case PILOT_STATE_ORIENT:
		case PILOT_STATE_MOVE:
			SetCondition(PILOT_ENGAGED);
			break;
		case PILOT_STATE_DONE:
			SetCondition(PILOT_IDLE);
			break;
		case PILOT_STATE_FAILED:
		case PILOT_STATE_ABORT:
			SetCondition(PILOT_FAILED);
			break;
		}

		pilotState = newState;
	}

	s = pthread_mutex_unlock(&pilotStateMtx);
	if (s != 0)
	{
		ERRORPRINT("pilot: pilotStateMtx unlock %i\n", s);
	}
	//end critical section

}
bool CancelPilotOperation(PilotState_enum newState)
{
	//cancel any current operation
	//assumes we already have the mutex
	//returns true if stop sent

	switch(pilotState)
	{
	case PILOT_STATE_FORWARD:
	case PILOT_STATE_BACKWARD:
	case PILOT_STATE_ORIENT:
	case PILOT_STATE_MOVE:
	case PILOT_STATE_FORWARD_SENT:
	case PILOT_STATE_BACKWARD_SENT:
	case PILOT_STATE_ORIENT_SENT:
	case PILOT_STATE_MOVE_SENT:
		//send stop message
		SendMotorCommand(0,0,0,0);
		DEBUGPRINT("pilot: %s Cancelled.\n", pilotStateNames[pilotState]);
		ChangePilotState(newState);
		return true;			//stop sent
		break;
	default:
		return false;
		break;
	}
}

//incoming actions - called from LUA Behavior Tree leaf actions
bool pilotEngaged = false;

ActionResult_enum AutopilotAction(PilotAction_enum _action)
{
	ActionResult_enum result = ACTION_FAIL;

	//critical section
	int s = pthread_mutex_lock(&pilotMtx);
	if (s != 0)
	{
		ERRORPRINT("pilot: pilotMtx lock %i\n", s);
	}
	DEBUGPRINT("pilot: Action: %s\n", pilotActionNames[_action]);

	switch(_action)
	{
	//simple actions
	case PILOT_FAST_SPEED:
		motorSpeed = FastSpeed;
		result = ACTION_SUCCESS;
		break;
	case PILOT_MEDIUM_SPEED:
		motorSpeed = MediumSpeed;
		result = ACTION_SUCCESS;
		break;
	case PILOT_SLOW_SPEED:
		motorSpeed = SlowSpeed;
		result = ACTION_SUCCESS;
		break;
	case PILOT_RESET:
		//cancel any current operation
		CancelPilotOperation(PILOT_STATE_IDLE);
		pilotEngaged = false;
		result = ACTION_SUCCESS;
		break;
	//the rest
	default:
	{
		if (pilotEngaged)		//currently working
		{
			//monitor on-going operation
			switch(pilotState)
			{
			case PILOT_STATE_FORWARD:		//short distance move using encoders only
			case PILOT_STATE_BACKWARD:		//ditto
			case PILOT_STATE_ORIENT:		//monitoring motion using compass
			case PILOT_STATE_MOVE:			//monitoring move using pose msg
			case PILOT_STATE_FORWARD_SENT:
			case PILOT_STATE_BACKWARD_SENT:
			case PILOT_STATE_ORIENT_SENT: 	//monitoring motion using compass
			case PILOT_STATE_MOVE_SENT:		//monitoring move using pose msg
				result = ACTION_RUNNING;
				break;
			case PILOT_STATE_DONE:			//move complete
			case PILOT_STATE_IDLE:			//ready for a command
				pilotEngaged = false;
				result =  ACTION_SUCCESS;
				break;
			case PILOT_STATE_FAILED:		//move failed
			case PILOT_STATE_ABORT:
				pilotEngaged = false;
				result = ACTION_FAIL;
				lastLuaCallReason = "Failed";
				break;
			case PILOT_STATE_INACTIVE:		//motors disabled
			default:
				pilotEngaged = false;
				result = ACTION_FAIL;
				lastLuaCallReason = "Inactive";
				break;
			}
		}
		else  //not currently working
			if (pilotState != PILOT_STATE_INACTIVE)		//available
		{
			//start new action
			//verify pre-requisites
			switch (_action) {
			case PILOT_ORIENT:
			case PILOT_TURN_LEFT:
			case PILOT_TURN_RIGHT:
			case PILOT_TURN_LEFT_90:
			case PILOT_TURN_RIGHT_90:
			case PILOT_TURN_N:
			case PILOT_TURN_S:
			case PILOT_TURN_E:
			case PILOT_TURN_W:
				if (motorsInhibit)
				{
					result = ACTION_FAIL;
					lastLuaCallReason = "Inhibit";
				}
				else if (!turnOK)
				{
					result = ACTION_FAIL;
					lastLuaCallReason = "Not OK";
				}
				else if (VerifyCompass()) {
					result = ACTION_RUNNING;
				}
				else
				{
					result = ACTION_FAIL;
					lastLuaCallReason = "Compass";
				}
				break;
			case PILOT_ENGAGE:
				if (motorsInhibit)
				{
					result = ACTION_FAIL;
					lastLuaCallReason = "Inhibit";
				}
				else if (!turnOK || !moveOK)
				{
						result = ACTION_FAIL;
						lastLuaCallReason = "Not OK";
				}
				else if (VerifyCompass() && VerifyGPS()) {
					result = ACTION_RUNNING;
				}
				else
				{
					result = ACTION_FAIL;
					lastLuaCallReason = "NoGPS";
				}

				break;
			case PILOT_MOVE_FORWARD:
			case PILOT_MOVE_BACKWARD:
			case PILOT_MOVE_FORWARD_10:
			case PILOT_MOVE_BACKWARD_10:
				if (motorsInhibit)
				{
					result = ACTION_FAIL;
					lastLuaCallReason = "Inhibit";
				}
				else if (!moveOK)
				{
					result = ACTION_FAIL;
					lastLuaCallReason = "Not OK";
				}
				else result = ACTION_RUNNING;
				break;
			default:
				ERRORPRINT("Invalid Pilot Action: %i\n", _action);
				result = ACTION_FAIL;
				lastLuaCallReason = "Bad Code";
				break;
			}

			if (result == ACTION_RUNNING)
			{
				//prepare goal
				//set up turns
				switch (_action) {
				case PILOT_ORIENT:
					desiredCompassHeading = ComputeBearing(nextPosition);
					break;
				case PILOT_TURN_LEFT:
					desiredCompassHeading = (360 + pose.orientation.heading - CalculateRandomTurn()) % 360;
					break;
				case PILOT_TURN_RIGHT:
					desiredCompassHeading = (pose.orientation.heading + CalculateRandomTurn()) % 360;
					break;
				case PILOT_TURN_LEFT_90:
					desiredCompassHeading = (pose.orientation.heading + 270) % 360;
					break;
				case PILOT_TURN_RIGHT_90:
					desiredCompassHeading = (pose.orientation.heading + 90) % 360;
					break;
				case PILOT_TURN_N:
					desiredCompassHeading = 0;
					break;
				case PILOT_TURN_S:
					desiredCompassHeading = 180;
					break;
				case PILOT_TURN_E:
					desiredCompassHeading = 90;
					break;
				case PILOT_TURN_W:
					desiredCompassHeading = 270;
					break;
				case PILOT_ENGAGE:
					desiredCompassHeading = ComputeBearing(nextPosition);
				default:
					break;
				}

				//start movement
				switch (_action) {
				case PILOT_ORIENT:
				case PILOT_TURN_LEFT:
				case PILOT_TURN_RIGHT:
				case PILOT_TURN_LEFT_90:
				case PILOT_TURN_RIGHT_90:
				case PILOT_TURN_N:
				case PILOT_TURN_S:
				case PILOT_TURN_E:
				case PILOT_TURN_W:
					DEBUGPRINT("Pilot: Orient to: %i\n", desiredCompassHeading);
					if (PerformOrient())
					{
						result = ACTION_SUCCESS;
					}
					else
					{
						ChangePilotState(PILOT_STATE_ORIENT_SENT);
					}
					break;
				case PILOT_ENGAGE:
					DEBUGPRINT("Pilot: Move to: %fN, %fE\n", nextPosition.longitude, nextPosition.latitude);
					if (PerformMovement())
					{
						result = ACTION_SUCCESS;
					}
					else
					{
						ChangePilotState(PILOT_STATE_MOVE_SENT);
					}
					break;
				case PILOT_MOVE_FORWARD:
				{
					int distance = CalculateRandomDistance();
					SendMotorCommand(distance, distance, SlowSpeed, pilotFlags);
					motorsRunTimeout = distance * timeoutPerCM + 5;
					DEBUGPRINT("Pilot: Move forward\n");
					ChangePilotState(PILOT_STATE_FORWARD_SENT);
				}
					break;
				case PILOT_MOVE_BACKWARD:
				{
					int distance = CalculateRandomDistance();
					SendMotorCommand(-distance, -distance, SlowSpeed, pilotFlags);
					motorsRunTimeout = distance * timeoutPerCM + 5;
					DEBUGPRINT("Pilot: Move backward\n");
					ChangePilotState(PILOT_STATE_BACKWARD_SENT);
				}
					break;
				case PILOT_MOVE_FORWARD_10:
				{
					int distance = 10;
					SendMotorCommand(distance, distance, SlowSpeed, pilotFlags);
					motorsRunTimeout = distance * timeoutPerCM + 5;
					DEBUGPRINT("Pilot: Move forward 10\n");
					ChangePilotState(PILOT_STATE_FORWARD_SENT);
				}
					break;
				case PILOT_MOVE_BACKWARD_10:
				{
					int distance = 10;
					SendMotorCommand(-distance, -distance, SlowSpeed, pilotFlags);
					motorsRunTimeout = distance * timeoutPerCM + 5;
					DEBUGPRINT("Pilot: Move backward 10\n");
					ChangePilotState(PILOT_STATE_BACKWARD_SENT);
				}
					break;
				default:
					break;
				}


			}
		}
		else
		{
			result = ACTION_FAIL;
			lastLuaCallReason = "Inactive";
		}
	}
	break;
	}
	if (result == ACTION_RUNNING) pilotEngaged = true;

	DEBUGPRINT("pilot: %s -> %s\n", pilotActionNames[_action], actionResultNames[result]);

	s = pthread_mutex_unlock(&pilotMtx);
	if (s != 0)
	{
		ERRORPRINT("Pilot: pilotMtx unlock %i\n", s);
	}
	//end critical section

	return result;
}

ActionResult_enum AutopilotIsReadyToMove()
{
	ActionResult_enum result;

	if (pilotState == PILOT_STATE_INACTIVE)
	{
		lastLuaCallReason = "Inactive";
		result = ACTION_FAIL;
	}
	else if (motorsInhibit)
	{
		result = ACTION_FAIL;
		lastLuaCallReason = "Inhibit";
	}
	else
	{
		result = ACTION_SUCCESS;
	}

	DEBUGPRINT("pilot: isReadyToMove -> %s\n", actionResultNames[result]);

	return result;
}

ActionResult_enum pilotIsAtWaypoint()
{
	ActionResult_enum result;

	float rangeCM = GetRangeToGoal();
	if (rangeCM < arrivalRange)
		result = ACTION_SUCCESS;
	else {
		lastLuaCallReason = "NotAtWP";
		result = ACTION_FAIL;
	}

	DEBUGPRINT("pilot: isAtWaypoint -> %s\n",  actionResultNames[result]);

	return result;
}

ActionResult_enum pilotSetGoalWaypoint(char *waypointName)
{
	ActionResult_enum result;
	DEBUGPRINT("pilot: SetGoalWaypoint(%s)\n", waypointName);

	if (!pose.positionValid) {
		lastLuaCallReason = "NoGPS";
		result = ACTION_FAIL;
	}
	else
	{
		AutopilotAction(PILOT_RESET);
		result = PlanPointToWaypoint(pose.position, waypointName);
	}
	DEBUGPRINT("pilot: SetGoalWaypoint(%s) -> %s\n", waypointName, actionResultNames[result]);
	return result;
}

ActionResult_enum pilotSetGoalPosition(Position_struct _goal)
{
	ActionResult_enum result;
	DEBUGPRINT("pilot: SetGoalPosn(%0.3f, %0.3f)\n", _goal.latitude, _goal.longitude);

	if (!pose.positionValid) {
		lastLuaCallReason = "NoGPS";
		result = ACTION_FAIL;
	}
	else
	{
		AutopilotAction(PILOT_RESET);
		result = PlanPointToPoint(pose.position, _goal);
	}
	DEBUGPRINT("pilot: SetGoalPosn(%0.3f, %0.3f) -> %s\n", _goal.latitude, _goal.longitude, actionResultNames[result]);
	return result;
}

ActionResult_enum pilotSetRandomGoal(int _rangeCM)
{
	ActionResult_enum result;
	DEBUGPRINT("Set Random Goal\n");

	if (!pose.positionValid){
		lastLuaCallReason = "NoGPS";
		result = ACTION_FAIL;
	}
	else
	{
		Position_struct goal;

		goal.latitude = pose.position.latitude + ((drand48() - 0.5) * _rangeCM * 2 * CM_PER_DEGREE);
		goal.longitude = pose.position.longitude + ((drand48() - 0.5) * _rangeCM * 2 * CM_PER_DEGREE);

		AutopilotAction(PILOT_RESET);
		result = PlanPointToPoint(pose.position, goal);
	}
	DEBUGPRINT("pilot: SetRandomGoal -> %s\n", actionResultNames[result]);
	return result;
}

//incoming messages - pick which ones to use
void AutopilotProcessMessage(psMessage_t *msg) {
	switch (msg->header.messageType) {

	case TICK_1S:
	case POSE:
	case ODOMETRY:
	case NOTIFY:
	case CONDITIONS:
		break;
	default:
		return;		//ignore other messages
		break;
	}
	CopyMessageToQ(&autopilotQueue, msg);
}

void SetVarFromCondition(psMessage_t *msg, Condition_enum e, bool *var)
{
	int bit 	= e % 64;
	int index 	= e / 64;

	NotificationMask_t mask = NOTIFICATION_MASK(bit);

	if (mask & msg->maskPayload.valid[index])
	{
		*var = (mask & msg->maskPayload.value[index]);
	}
}

//thread to send updates to the motor processor
void *AutopilotThread(void *arg) {
	bool reviewProgress;

	PowerState_enum powerState = POWER_STATE_UNKNOWN;

	DEBUGPRINT("pilot: thread ready\n");

	//loop
	while (1) {
		psMessage_t *rxMessage = GetNextMessage(&autopilotQueue);

		reviewProgress = false;

		//critical section
		int s = pthread_mutex_lock(&pilotMtx);
		if (s != 0)
		{
			ERRORPRINT("pilot: pilotMtx lock %i\n", s);
		}

		//process message
		switch (rxMessage->header.messageType) {
		case TICK_1S:
			powerState = rxMessage->tickPayload.systemPowerState;
			if (powerState < POWER_ACTIVE)
			{
				//shutting down
				DEBUGPRINT("pilot: Tick -> Shutdown\n");
				CancelPilotOperation(PILOT_STATE_INACTIVE);
			}
			else
			{
				//make pilot available
				if ((moveOK || turnOK) && !motorsInhibit && (pilotState == PILOT_STATE_INACTIVE) && MOTonline && MCPonline)
				{
					DEBUGPRINT("pilot: Tick -> Available\n");
					ChangePilotState(PILOT_STATE_IDLE);
				}
			}
			reviewProgress = true;

			switch (pilotState)
			{
			case PILOT_STATE_FORWARD_SENT:
			case PILOT_STATE_BACKWARD_SENT:
			case PILOT_STATE_ORIENT_SENT:
			case PILOT_STATE_MOVE_SENT:

				MOVE_XXX_time = 0;
				if (MOVE_XXX_SENT_time == 0)
					MOVE_XXX_SENT_time = time(NULL);
				else
				{
					if (MOVE_XXX_SENT_time + motorsStartTimeout < time(NULL))
					{
						MOVE_XXX_SENT_time = time(NULL);
						ERRORPRINT("pilot: Motors Start TO\n");
						LogWarning("Motors Start TO\n");
						ResendMotorCommand();
					}
				}
				break;
			case PILOT_STATE_FORWARD:
			case PILOT_STATE_BACKWARD:
			case PILOT_STATE_ORIENT:
			case PILOT_STATE_MOVE:

				MOVE_XXX_SENT_time = 0;
				if (MOVE_XXX_time == 0)
					MOVE_XXX_time = time(NULL);
				else
				{
					if (MOVE_XXX_time + motorsRunTimeout < time(NULL))
					{
						ERRORPRINT("pilot: Motors Run TO\n");
						CancelPilotOperation(PILOT_STATE_FAILED);
						lastLuaCallReason = "RunTO";
						MOVE_XXX_time =  time(NULL);
					}
				}
				break;
			default:
				MOVE_XXX_time = 0;
				MOVE_XXX_SENT_time = 0;
				break;
			}
			break;
		case POSE:
//			DEBUGPRINT("pilot: POSE message\n");
			pose = rxMessage->posePayload;
			gettimeofday(&latestPoseTime, NULL);
			reviewProgress = true;
			break;
		case ODOMETRY:
			DEBUGPRINT("pilot: ODOMETRY message\n");
			odometry = rxMessage->odometryPayload;
			gettimeofday(&latestOdoTime, NULL);
			reviewProgress = true;
			break;
		case NOTIFY:
		{
			Event_enum event = rxMessage->intPayload.value;
			switch(event)
			{
			case FRONT_CONTACT_EVENT:
				if (pilotFlags & (ENABLE_FRONT_CONTACT_ABORT)) {
					DEBUGPRINT("pilot: Front Contact Stop\n");
					CancelPilotOperation(PILOT_STATE_FAILED);
					lastLuaCallReason = "BumpFront";
				}
				break;
			case REAR_CONTACT_EVENT:
				if (pilotFlags & (ENABLE_REAR_CONTACT_ABORT)){
					DEBUGPRINT("pilot: Rear Contact Stop\n");
					CancelPilotOperation(PILOT_STATE_FAILED);
					lastLuaCallReason = "BumpRear";
				}
				break;
			case SLEEPING_EVENT:
				DEBUGPRINT("pilot: Sleeping Stop\n");
				CancelPilotOperation(PILOT_STATE_FAILED);
				lastLuaCallReason = "Sleeping";
				break;
			case BATTERY_SHUTDOWN_EVENT:
				DEBUGPRINT("pilot: Battery Shutdown Stop\n");
				CancelPilotOperation(PILOT_STATE_FAILED);
				lastLuaCallReason = "Battery";
				break;
			case BATTERY_CRITICAL_EVENT:
				if (pilotFlags & ENABLE_SYSTEM_ABORT) {
					DEBUGPRINT("pilot: Battery Critical Stop\n");
					CancelPilotOperation(PILOT_STATE_FAILED);
					lastLuaCallReason = "Battery";
				}
				break;
			case LOST_FIX_EVENT:
				if (pilotFlags & ENABLE_LOSTFIX_ABORT) {
					DEBUGPRINT("pilot: Lost Fix Stop\n");
					CancelPilotOperation(PILOT_STATE_FAILED);
					lastLuaCallReason = "Lost Fix";
				}
				break;
			default:
				break;
			}
		}
			break;

		case CONDITIONS:
		{
			NotificationMask_t changedConditions = (currentConditions & rxMessage->maskPayload.valid[0]) ^ ( rxMessage->maskPayload.value[0] & rxMessage->maskPayload.valid[0]);
			currentConditions |= rxMessage->maskPayload.value[0] & rxMessage->maskPayload.valid[0];
			currentConditions &= ~(~rxMessage->maskPayload.value[0] & rxMessage->maskPayload.valid[0]);

			if ((currentConditions & frontCloseMask) && (pilotFlags & ENABLE_FRONT_CLOSE_ABORT)){		//assumes all flags < 64
				if (CancelPilotOperation(PILOT_STATE_FAILED))
				{
					lastLuaCallReason = "ProxFront";
					DEBUGPRINT("pilot: Front Prox Stop\n");
				}
			}
			if ((currentConditions & rearCloseMask) && (pilotFlags & ENABLE_REAR_CLOSE_ABORT)) {
				if (CancelPilotOperation(PILOT_STATE_FAILED))
				{
					lastLuaCallReason = "ProxRear";
					DEBUGPRINT("pilot: Rear Prox Stop\n");
				}
			}
			if ((currentConditions & frontFarMask) && (pilotFlags & ENABLE_FRONT_FAR_ABORT)){
				if (CancelPilotOperation(PILOT_STATE_FAILED))
				{
					lastLuaCallReason = "ProxFar";
					DEBUGPRINT("pilot: Far Prox Stop");
				}
			}

			motorsInhibit = currentConditions & NOTIFICATION_MASK(MOTORS_INHIBIT);
			motorsBusy = currentConditions & NOTIFICATION_MASK(MOTORS_BUSY);
			motorsErrors = currentConditions & NOTIFICATION_MASK(MOTORS_ERRORS);

			if (motorsInhibit)
			{
				if (CancelPilotOperation(PILOT_STATE_INACTIVE))
				{
					lastLuaCallReason = "MotInhibit";
					DEBUGPRINT("pilot: Motor Inhibit Stop");
				}
			}
			if (NOTIFICATION_MASK(MOTORS_BUSY) & changedConditions)
			{
				if (motorsBusy)
				{
					//confirm motors started
					switch (pilotState)
					{
					case PILOT_STATE_FORWARD_SENT:
						DEBUGPRINT("pilot: Move Fwd Started\n");
						ChangePilotState(PILOT_STATE_FORWARD);
						break;
					case PILOT_STATE_BACKWARD_SENT:
						DEBUGPRINT("pilot: Move Back Started\n");
						ChangePilotState(PILOT_STATE_BACKWARD);
						break;
					case PILOT_STATE_ORIENT_SENT:
						DEBUGPRINT("pilot: Orient Started\n");
						ChangePilotState(PILOT_STATE_ORIENT);
						break;
					case PILOT_STATE_MOVE_SENT:
						DEBUGPRINT("pilot: Move Started\n");
						ChangePilotState(PILOT_STATE_MOVE);
						break;
					default:
						break;
					}
				}
				else
				{
					//motors stopped; review
					switch (pilotState)
					{
					case PILOT_STATE_FORWARD:
					case PILOT_STATE_BACKWARD:
						DEBUGPRINT("pilot: Move Done\n");
						ChangePilotState(PILOT_STATE_DONE);
						break;
					case PILOT_STATE_ORIENT:
						if (deadReckon)
						{
							DEBUGPRINT("pilot: D/R Orient Done\n");
							ChangePilotState(PILOT_STATE_DONE);
						}
						else
						{
							PerformOrient();
						}
						break;
					case PILOT_STATE_MOVE:
						PerformMovement();
					default:
						break;
					}
				}
			}
			break;
		}
		default:
			break;
		}

		s = pthread_mutex_unlock(&pilotMtx);
		if (s != 0)
		{
			ERRORPRINT("Pilot: pilotMtx unlock %i\n", s);
		}
		//end critical section

		DoneWithMessage(rxMessage);
	}
}

bool VerifyCompass()
{
	if (!pose.orientationValid && !deadReckon) {
		DEBUGPRINT("pilot: No compass fail\n");
		ChangePilotState(PILOT_STATE_FAILED);
		lastLuaCallReason = "Compass";
		return false;
	}
	return true;
}

bool VerifyGPS()
{
	if (!pose.positionValid) {
		DEBUGPRINT("pilot: No GPS fail");
		ChangePilotState(PILOT_STATE_FAILED);
		lastLuaCallReason = "NoGPS";
		return false;
	}
	return true;
}

int ComputeBearing(Position_struct _position)
{
	int bearing = fmod(RADIANSTODEGREES(
			atan2(_position.longitude - pose.position.longitude,
					_position.latitude - pose.position.latitude)) + 360.0, 360.0);

	DEBUGPRINT("pilot: compute bearing = %i\n", bearing);

	return bearing;
}


bool PerformOrient()
{
	//initiate turn to 'desiredCompassHeading'
	//calculate angle error
	int angleError = (360 + desiredCompassHeading
			- pose.orientation.heading) % 360;
	if (angleError > 180)
		angleError -= 360;

	//if close, report done
	if (abs(angleError) < arrivalHeading) {
		SendMotorCommand(0,0,0,0);
		DEBUGPRINT("pilot: Orient done\n");
		ChangePilotState(PILOT_STATE_DONE);
		return true;
	} else {
		DEBUGPRINT("pilot: heading = %i, bearing = %i, error = %i\n", pose.orientation.heading, desiredCompassHeading, angleError);
		//send turn command
		int range = abs((angleError * FIDO_RADIUS * M_PI) / 180) * movePercentage;
		//send turn command
		if (angleError > 0) {
			SendMotorCommand(range, -range, SlowSpeed, pilotFlags);
		} else {
			SendMotorCommand(-range, range, SlowSpeed, pilotFlags);
		}
		motorsRunTimeout = range * timeoutPerCM + 10;
		return false;
	}
}

float GetRangeToGoal()
{
	//check range
	double latitudeDifference = (nextPosition.latitude
			- pose.position.latitude);
	double longitudeDifference = (nextPosition.longitude
			- pose.position.longitude);

	double range = sqrt(
			latitudeDifference * latitudeDifference
			+ longitudeDifference * longitudeDifference);

	return (range * CM_PER_DEGREE);
}

bool PerformMovement()
{
	double rangeCM = GetRangeToGoal();

	DEBUGPRINT("pilot: Goal Range = %.0f\n", rangeCM);

	while (rangeCM < arrivalRange)
	{
		SendMotorCommand(0,0,0,0);
		if (++routeWaypointIndex >= planWaypointCount)
		{
			DEBUGPRINT("pilot: Route done\n");
			ChangePilotState(PILOT_STATE_DONE);
			return true;
		}
		else
		{
			if (pilotFlags & ENABLE_WAYPOINT_STOP)
				{
					DEBUGPRINT("pilot: Waypoint done\n");
					ChangePilotState(PILOT_STATE_DONE);
					return true;
				}

			nextWaypointName 				= planWaypoints[routeWaypointIndex];
			Waypoint_struct *nextWaypoint	= GetWaypointByName(nextWaypointName);
			if (nextWaypoint)
			{
				nextPosition = nextWaypoint->position;
			}
			else
			{
				ERRORPRINT("pilot: Can't find next WP: %s\n\n", nextWaypointName);
				ChangePilotState(PILOT_STATE_FAILED);
				lastLuaCallReason = "NextWP";
				return true;
			}
		}
		rangeCM = GetRangeToGoal();
	}

	//start required orientation
	desiredCompassHeading = ComputeBearing(nextPosition);

	if (PerformOrient())
	{
		//no turn needed
		//send move command
		int motorRange = (rangeCM > motorMaxCM ? motorMaxCM : rangeCM);

		if (moveOK) SendMotorCommand(motorRange, motorRange, motorSpeed, pilotFlags);
		motorsRunTimeout = motorRange * timeoutPerCM + 5;

		//TODO: Adjust port & starboard to correct residual heading error
	}
	return false;
}

int lastsent_port, lastsent_starboard;
float lastsent_speed;
uint16_t lastsent_flags;

void SendMotorCommand(int _port, int _starboard, float _speed, uint16_t _flags)
{
	psMessage_t motorMessage;
	psInitPublish(motorMessage, MOTORS);	//prepare message to motors
	motorMessage.motorPayload.portMotors = _port;
	motorMessage.motorPayload.starboardMotors = _starboard;
	motorMessage.motorPayload.flags = _flags & 0xff;		//flags is uint8_t
	motorMessage.motorPayload.speed = (int)( _speed / 50.0);

	DEBUGPRINT("pilot: Motor cmd: %i, %i, speed=%0.0f\n", _port, _starboard, _speed);

	RouteMessage(&motorMessage);

	lastsent_port 		= _port;
	lastsent_starboard 	= _starboard;
	lastsent_speed 		= _speed;
	lastsent_flags		= _flags;
}
void ResendMotorCommand()
{
	SendMotorCommand(lastsent_port, lastsent_starboard, lastsent_speed, lastsent_flags);
}

//convenience routines
int CalculateRandomDistance()
{
	float move = defMove + RANDOM_MOVE_SPREAD * (drand48() - 0.5);
	return (int) move;
}
int CalculateRandomTurn()
{
	float turn = defTurn + RANDOM_TURN_SPREAD * (drand48() - 0.5);
	return (int) turn;
}

//route planning vars

//latest pose report
psPosePayload_t pose;
struct timeval latestPoseTime;

//latest odometry message
psOdometryPayload_t odometry;
struct timeval latestOdoTime = { 0, 0 };

//endpint of route
Waypoint_struct *goalWaypoint = NULL;
Position_struct goalPosition = {0.0,0.0};

//route waypoints
int planWaypointCount = 0;
char **planWaypoints = NULL;

float totalPlanCost = 0.0;

//next waypoint and position
int routeWaypointIndex = 0;
char *nextWaypointName = NULL;
Position_struct nextPosition = {0.0,0.0};
int desiredCompassHeading = 0;
