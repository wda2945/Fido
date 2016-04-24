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

bool VerifyCompass();
bool VerifyGPS();
int ComputeBearing(Position_struct _position);
//for new movement commands
bool ProcessActionCommand(PilotAction_enum _action);
//start/continue action
bool PerformOrient();
bool PerformMovement();
//send motor command
void SendMotorCommand(int _port, int _starboard, int _speed, uint16_t _flags);

#define RANDOM_MOVE_SPREAD 20	//cm
int CalculateRandomDistance();
#define RANDOM_TURN_SPREAD 30	//degrees
int CalculateRandomTurn();

//autopilot state machine
typedef enum {
	PILOT_STATE_IDLE,			//ready for a command
	PILOT_STATE_FORWARD_SENT,	//short distance move using encoders only
	PILOT_STATE_BACKWARD_SENT,	//ditto
	PILOT_STATE_ORIENT_SENT,		//monitoring motion using compass
	PILOT_STATE_MOVE_SENT,			//monitoring move using pose msg
	PILOT_STATE_FORWARD,		//short distance move using encoders only
	PILOT_STATE_BACKWARD,		//ditto
	PILOT_STATE_ORIENT,		//monitoring motion using compass
	PILOT_STATE_MOVE,			//monitoring move using pose msg
	PILOT_STATE_DONE,			//move complete
	PILOT_STATE_FAILED,			//move failed
	PILOT_STATE_ABORT,
	PILOT_STATE_INACTIVE		//motors disabled
} PilotState_enum;

static char *pilotStateNames[] = {	"IDLE",			//ready for a command
		"FORWARD_SENT",			//short distance move using encoders only
		"BACKWARD_SENT",		//ditto
		"ORIENT_SENT",		//monitoring motion using compass
		"MOVE_SENT",			//monitoring move using pose msg
		"FORWARD",				//short distance move using encoders only
		"BACKWARD",				//ditto
		"ORIENT",			//monitoring motion using compass
		"MOVE",				//monitoring move using pose msg
		"DONE",					//move complete
		"FAILED",				//move failed
		"ABORT",
		"INACTIVE"				//motors disabled
};

PilotState_enum pilotState = PILOT_STATE_INACTIVE;

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
int motorSpeed;

#define CM_PER_DEGREE (60.0 * 5280.0 * 12.0 * 2.54)

int AutopilotInit() {

	pilotDebugFile = fopen("/root/logfiles/pilot.log", "w");

	//load waypoints
	LoadWaypointDatabase(WAYPOINT_FILE_PATH);

	if (InitNodeList() < 0)
	{
		ERRORPRINT("InitNodeList fail");
		return -1;
	}

	//create autopilot thread
	pthread_t thread;
	int s = pthread_create(&thread, NULL, AutopilotThread, NULL);
	if (s != 0) {
		ERRORPRINT("Pilot Thread fail %i", s);
		return -1;
	}

	return 0;
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
		DEBUGPRINT("Pilot %s Cancelled.\n", pilotStateNames[pilotState]);
		pilotState = newState;
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
	int s = pthread_mutex_lock(&pilotStateMtx);
	if (s != 0)
	{
		ERRORPRINT("Pilot: pilotStateMtx lock %i\n", s);
	}

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
	default:
	{
		if (pilotEngaged)
		{
			//monitor on-going operation
			switch(pilotState)
			{
			case PILOT_STATE_FORWARD:			//short distance move using encoders only
			case PILOT_STATE_BACKWARD:			//ditto
			case PILOT_STATE_ORIENT:			//monitoring motion using compass
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
				pilotEngaged = false;
				result = ACTION_FAIL;
				break;
			case PILOT_STATE_INACTIVE:		//motors disabled
			default:
				pilotEngaged = false;
				result = ACTION_FAIL;
				lastLuaCallReason = "Inactive";
				break;
			}
		}
		else if (pilotState != PILOT_STATE_INACTIVE)
		{
			//start new action
			//verify pre-requisites
			switch (_action) {
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
				result = ACTION_SUCCESS;
				break;
			case PILOT_ORIENT:
			case PILOT_TURN_LEFT:
			case PILOT_TURN_RIGHT:
			case PILOT_TURN_LEFT_90:
			case PILOT_TURN_RIGHT_90:
			case PILOT_TURN_N:
			case PILOT_TURN_S:
			case PILOT_TURN_E:
			case PILOT_TURN_W:
				if (VerifyCompass()) {
					result = ACTION_RUNNING;
				}
				else
				{
					result = ACTION_FAIL;
					lastLuaCallReason = "Compass";
				}
				break;
			case PILOT_ENGAGE:
				if (VerifyCompass() && VerifyGPS()) {
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
				result = ACTION_RUNNING;
				break;
			default:
				LogError("Pilot action: %i\n", _action);
				result = ACTION_FAIL;
				lastLuaCallReason = "BadCode";
				break;
			}

			if (result == ACTION_RUNNING)
			{
				//prepare goal
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
					desiredCompassHeading = (pose.orientation.heading - 90) % 360;
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
						pilotState = PILOT_STATE_ORIENT_SENT;
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
						pilotState = PILOT_STATE_MOVE_SENT;
					}
					break;
				case PILOT_MOVE_FORWARD:
				{
					int distance = CalculateRandomDistance();
					SendMotorCommand(distance, distance, SlowSpeed, pilotFlags);
					motorsRunTimeout = distance * timeoutPerCM + 5;
					DEBUGPRINT("Pilot: Move forward\n");
					pilotState = PILOT_STATE_FORWARD_SENT;
				}
					break;
				case PILOT_MOVE_BACKWARD:
				{
					int distance = CalculateRandomDistance();
					SendMotorCommand(-distance, -distance, SlowSpeed, pilotFlags);
					motorsRunTimeout = distance * timeoutPerCM + 5;
					DEBUGPRINT("Pilot: Move backward\n");
					pilotState = PILOT_STATE_BACKWARD_SENT;
				}
					break;
				case PILOT_MOVE_FORWARD_10:
				{
					int distance = 10;
					SendMotorCommand(distance, distance, SlowSpeed, pilotFlags);
					motorsRunTimeout = distance * timeoutPerCM + 5;
					DEBUGPRINT("Pilot: Move forward 10\n");
					pilotState = PILOT_STATE_FORWARD_SENT;
				}
					break;
				case PILOT_MOVE_BACKWARD_10:
				{
					int distance = 10;
					SendMotorCommand(-distance, -distance, SlowSpeed, pilotFlags);
					motorsRunTimeout = distance * timeoutPerCM + 5;
					DEBUGPRINT("Pilot: Move backward 10\n");
					pilotState = PILOT_STATE_BACKWARD_SENT;
				}
					break;
				default:
					break;
				}

				if (result == ACTION_RUNNING) pilotEngaged = true;
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

	s = pthread_mutex_unlock(&pilotStateMtx);
	if (s != 0)
	{
		ERRORPRINT("Pilot: pilotStateMtx unlock %i\n", s);
	}
	//end critical section

	return result;
}

ActionResult_enum AutopilotIsReadyToMove()
{
	ActionResult_enum result = ACTION_SUCCESS;
	//critical section
	int s = pthread_mutex_lock(&pilotStateMtx);
	if (s != 0)
	{
		ERRORPRINT("Pilot: pilotStateMtx lock %i\n", s);
	}

	if (pilotState == PILOT_STATE_INACTIVE)
	{
		lastLuaCallReason = "Inactive";
		result = ACTION_FAIL;
	}

	s = pthread_mutex_unlock(&pilotStateMtx);
	if (s != 0)
	{
		ERRORPRINT("Pilot: pilotStateMtx unlock %i\n", s);
	}
	//end critical section

	return result;
}

ActionResult_enum pilotIsAtWaypoint()
{

	float rangeCM = GetRangeToGoal();
	if (rangeCM < arrivalRange)
		return ACTION_SUCCESS;
	else {
		lastLuaCallReason = "NotAtWP";
		return ACTION_FAIL;
	}

}

ActionResult_enum pilotSetGoalWaypoint(char *waypointName)
{
	if (!pose.positionValid) {
		lastLuaCallReason = "NoGPS";
		return ACTION_FAIL;
	}

	AutopilotAction(PILOT_RESET);

	return PlanPointToWaypoint(pose.position, waypointName);
}

ActionResult_enum pilotSetGoalPosition(Position_struct _goal)
{
	if (!pose.positionValid) {
		lastLuaCallReason = "NoGPS";
		return ACTION_FAIL;
	}
	AutopilotAction(PILOT_RESET);
	return PlanPointToPoint(pose.position, _goal);
}

ActionResult_enum pilotSetRandomGoal(int _rangeCM)
{
	if (!pose.positionValid){
		lastLuaCallReason = "NoGPS";
		return ACTION_FAIL;
	}

	Position_struct goal;

	goal.latitude = pose.position.latitude + ((drand48() - 0.5) * _rangeCM * 2 * CM_PER_DEGREE);
	goal.longitude = pose.position.longitude + ((drand48() - 0.5) * _rangeCM * 2 * CM_PER_DEGREE);

	AutopilotAction(PILOT_RESET);
	return PlanPointToPoint(pose.position, goal);
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

	int priorPilotState;	//used to cancel notifications

	PowerState_enum powerState = POWER_STATE_UNKNOWN;

	DEBUGPRINT("Pilot thread ready\n");

	//loop
	while (1) {
		psMessage_t *rxMessage = GetNextMessage(&autopilotQueue);

		DEBUGPRINT("Pilot Msg: %s\n", psLongMsgNames[rxMessage->header.messageType]);

		reviewProgress = false;

		//critical section
		int s = pthread_mutex_lock(&pilotStateMtx);
		if (s != 0)
		{
			ERRORPRINT("Pilot: pilotStateMtx lock %i\n", s);
		}

		priorPilotState = pilotState;	//track state for Conditions

		//process message
		switch (rxMessage->header.messageType) {
		case TICK_1S:
			powerState = rxMessage->tickPayload.systemPowerState;
			if (powerState < POWER_ACTIVE)
			{
				//shutting down
				CancelPilotOperation(PILOT_STATE_INACTIVE);
				DEBUGPRINT("Pilot Shutdown\n");			}
			else
			{
				//make pilot available
				if (moveOK && !motorsInhibit && (pilotState == PILOT_STATE_INACTIVE) && MOTonline && MCPonline)
				{
					pilotState = PILOT_STATE_IDLE;
					DEBUGPRINT("Pilot Available\n");
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
						CancelPilotOperation(PILOT_STATE_FAILED);
						lastLuaCallReason = "StartTO";
						MOVE_XXX_SENT_time = 0;
						LogWarning("Motors Start TO\n");
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
						CancelPilotOperation(PILOT_STATE_FAILED);
						lastLuaCallReason = "RunTO";
						MOVE_XXX_time = 0;
						LogWarning("Motors Run TO\n");
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
			pose = rxMessage->posePayload;
			gettimeofday(&latestPoseTime, NULL);
			reviewProgress = true;
			break;
		case ODOMETRY:
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
					CancelPilotOperation(PILOT_STATE_FAILED);
					lastLuaCallReason = "BumpFront";
					DEBUGPRINT("Pilot Front Contact Stop\n");
				}
				break;
			case REAR_CONTACT_EVENT:
				if (pilotFlags & (ENABLE_REAR_CONTACT_ABORT)){
					CancelPilotOperation(PILOT_STATE_FAILED);
					lastLuaCallReason = "BumpRear";
					DEBUGPRINT("Pilot Rear Contact Stop\n");
				}
				break;
			case SLEEPING_EVENT:
				CancelPilotOperation(PILOT_STATE_FAILED);
				lastLuaCallReason = "Sleeping";
				DEBUGPRINT("Pilot System Stop\n");
				break;
			case BATTERY_SHUTDOWN_EVENT:
				CancelPilotOperation(PILOT_STATE_FAILED);
				lastLuaCallReason = "Battery";
				DEBUGPRINT("Pilot Shutdown Stop\n");
				break;
			case BATTERY_CRITICAL_EVENT:
				if (pilotFlags & ENABLE_SYSTEM_ABORT) {
					CancelPilotOperation(PILOT_STATE_FAILED);
					lastLuaCallReason = "Battery";
					DEBUGPRINT("Pilot Critical Stop\n");
				}
				break;
			case LOST_FIX_EVENT:
				if (pilotFlags & ENABLE_LOSTFIX_ABORT) {
					CancelPilotOperation(PILOT_STATE_FAILED);
					lastLuaCallReason = "Lost Fix";
					DEBUGPRINT("Pilot Lost Fix Stop\n");
				}
				break;
			default:
				break;
			}
		}
			break;

		case CONDITIONS:
		{
			currentConditions |= rxMessage->maskPayload.value[0] & rxMessage->maskPayload.valid[0];
			currentConditions &= ~(~rxMessage->maskPayload.value[0] & rxMessage->maskPayload.valid[0]);

			if ((currentConditions & frontCloseMask) && (pilotFlags & ENABLE_FRONT_CLOSE_ABORT)){		//assumes all flags < 64
				if (CancelPilotOperation(PILOT_STATE_FAILED))
				{
					lastLuaCallReason = "ProxFront";
					DEBUGPRINT("Pilot Front Prox Stop\n");
				}
			}
			if ((currentConditions & rearCloseMask) && (pilotFlags & ENABLE_REAR_CLOSE_ABORT)) {
				if (CancelPilotOperation(PILOT_STATE_FAILED))
				{
					lastLuaCallReason = "ProxRear";
					DEBUGPRINT("Pilot Rear Prox Stop\n");
				}
			}
			if ((currentConditions & frontFarMask) && (pilotFlags & ENABLE_FRONT_FAR_ABORT)){
				if (CancelPilotOperation(PILOT_STATE_FAILED))
				{
					lastLuaCallReason = "ProxFar";
					DEBUGPRINT("Pilot Far Prox Stop");
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
					DEBUGPRINT("Pilot Motor Inhibit Stop");
				}
			}
			if (motorsBusy)
			{
				switch (pilotState)
				{
				case PILOT_STATE_FORWARD_SENT:
					pilotState = PILOT_STATE_FORWARD;
					DEBUGPRINT("Pilot Move Fwd Started\n");
					break;
				case PILOT_STATE_BACKWARD_SENT:
					pilotState = PILOT_STATE_BACKWARD;
					DEBUGPRINT("Pilot Move Back Started\n");
					break;
				case PILOT_STATE_ORIENT_SENT:
					pilotState = PILOT_STATE_ORIENT;
					DEBUGPRINT("Pilot Orient Started\n");
					break;
				case PILOT_STATE_MOVE_SENT:
					pilotState = PILOT_STATE_MOVE;
					DEBUGPRINT("Pilot Move Started\n");
					break;
				default:
					break;
				}
			}
			else
			{
				switch (pilotState)
				{
				case PILOT_STATE_FORWARD:
				case PILOT_STATE_BACKWARD:
					pilotState = PILOT_STATE_DONE;
					DEBUGPRINT("Pilot Move Done\n");
					break;
				case PILOT_STATE_ORIENT:
					if (deadReckon)
					{
						pilotState = PILOT_STATE_DONE;
						DEBUGPRINT("D/R Orient Done\n");
					}
					else
					{
						reviewProgress = true;
					}
					break;
				case PILOT_STATE_MOVE:
					reviewProgress = true;
				default:
					break;
				}
			}

			break;
		}
		default:
			break;
		}

		if (reviewProgress & ~motorsBusy)
		{
			//review what's happening
			switch (pilotState)
			{
			//Orienting is just a turn to a specific compass direction
			case PILOT_STATE_ORIENT:
				PerformOrient();
				break;

			case PILOT_STATE_MOVE:
				PerformMovement();
				break;
			default:
				break;
			}
		}

		//update notifications
		if (priorPilotState != pilotState) {
			DEBUGPRINT("New Pilot State: %s", pilotStateNames[pilotState]);

			switch (priorPilotState) {
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
				CancelCondition(PILOT_FAILED);
				break;
			}

			switch (pilotState) {
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
				SetCondition(PILOT_FAILED);
				break;
			}
		}

		s = pthread_mutex_unlock(&pilotStateMtx);
		if (s != 0)
		{
			ERRORPRINT("Pilot: pilotStateMtx unlock %i\n", s);
		}
		//end critical section

		DoneWithMessage(rxMessage);
	}
}

bool VerifyCompass()
{
	if (!pose.orientationValid && !deadReckon) {
		DEBUGPRINT("Pilot: No compass fail\n");
		pilotState = PILOT_STATE_FAILED;
		lastLuaCallReason = "Compass";
		return false;
	}
	return true;
}

bool VerifyGPS()
{
	if (!pose.positionValid) {
		DEBUGPRINT("Pilot: No GPS fail");
		pilotState = PILOT_STATE_FAILED;
		lastLuaCallReason = "NoGPS";
		return false;
	}
	return true;
}

int ComputeBearing(Position_struct _position)
{
	return (int) RADIANSTODEGREES(
			atan2(_position.longitude - pose.position.longitude,
					_position.latitude - pose.position.latitude));
}


bool PerformOrient()
{
	//initiate turn to 'desiredCompassHeading'
	//calculate angle error
	float angleError = (360 + desiredCompassHeading
			- pose.orientation.heading) % 360;
	if (angleError > 180)
		angleError -= 360;

	//if close, report done
	if (abs(angleError) < arrivalHeading) {
		SendMotorCommand(0,0,0,0);
		DEBUGPRINT("Pilot: Orient done\n");
		pilotState = PILOT_STATE_DONE;
		return true;
	} else {
		DEBUGPRINT("Pilot: angle error %f\n", angleError);
		//send turn command
		float range = abs(angleError * FIDO_RADIUS * M_PI / 180.0);
		//send turn command
		if (angleError > 0) {
			SendMotorCommand((int)range, (int)-range, SlowSpeed, pilotFlags);
		} else {
			SendMotorCommand((int)-range, (int)range, SlowSpeed, pilotFlags);
		}
		motorsRunTimeout = range * timeoutPerCM + 5;
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

	return (float) (range * CM_PER_DEGREE);
}

bool PerformMovement()
{
	float rangeCM = GetRangeToGoal();

	DEBUGPRINT("Pilot: Range = %.0f\n", rangeCM);

	while (rangeCM < arrivalRange)
	{
		SendMotorCommand(0,0,0,0);
		if (++routeWaypointIndex >= planWaypointCount)
		{
			DEBUGPRINT("Pilot: Route done\n");
			pilotState = PILOT_STATE_DONE;
			return true;
		}
		else
		{
			if (pilotFlags & ENABLE_WAYPOINT_STOP)
				{
					DEBUGPRINT("Pilot: Waypoint done\n");
					pilotState = PILOT_STATE_DONE;
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
				LogWarning("Pilot: Can't find next WP: %s\n\n", nextWaypointName);
				pilotState = PILOT_STATE_FAILED;
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

		SendMotorCommand(motorRange, motorRange, motorSpeed, pilotFlags);
		motorsRunTimeout = motorRange * timeoutPerCM + 5;

		//TODO: Adjust port & starboard to correct residual heading error
	}
	return false;
}


void SendMotorCommand(int _port, int _starboard, int _speed, uint16_t _flags)
{
	psMessage_t motorMessage;
	psInitPublish(motorMessage, MOTORS);	//prepare message to motors
	motorMessage.motorPayload.portMotors = _port;
	motorMessage.motorPayload.starboardMotors = _starboard;
	motorMessage.motorPayload.flags = _flags & 0xff;		//flags is uint8_t
	motorMessage.motorPayload.speed = _speed;
	RouteMessage(&motorMessage);
//	gettimeofday(&latestMotorTime, NULL);

	DEBUGPRINT("Pilot: Motors %i, %i\n", _port, _starboard);
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
