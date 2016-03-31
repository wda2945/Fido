/*
 * autopilot_common.h
 *
 *  Created on: 2015
 *      Author: martin
 */

#ifndef AUTOPILOT_COMMON_H_
#define AUTOPILOT_COMMON_H_

#include "PubSubData.h"
#include "Waypoints.h"

extern FILE *pilotDebugFile;

#ifdef AUTOPILOT_DEBUG
#define DEBUGPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(pilotDebugFile, __VA_ARGS__);fflush(pilotDebugFile);
#else
#define DEBUGPRINT(...) fprintf(pilotDebugFile, __VA_ARGS__);fflush(pilotDebugFile);
#endif

#define ERRORPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(pilotDebugFile, __VA_ARGS__);fflush(pilotDebugFile);

float GetRangeToGoal();

//latest pose report
extern psPosePayload_t pose;
extern struct timeval latestPoseTime;

//latest odometry message
extern psOdometryPayload_t odometry;
extern struct timeval latestOdoTime;

//final target
extern Waypoint_struct *goalWaypoint;
extern Position_struct goalPosition;

//the plan
extern int planWaypointCount;
extern char **planWaypoints;
extern float totalPlanCost;

//next step on the plan
extern int routeWaypointIndex;
extern char *nextWaypointName;
extern Position_struct nextPosition;
extern int desiredCompassHeading;



#endif
