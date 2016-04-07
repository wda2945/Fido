/*
 * planner.h
 *
 *  Created on: 2016
 *      Author: martin
 */


//Route planning

#ifndef PLANNER_H_
#define PLANNER_H_

#include "pubsubdata.h"
#include "behavior/behavior_enums.h"

#ifdef __cplusplus
extern "C" {
#endif


int InitNodeList();

ActionResult_enum PlanWaypointToWaypoint(char *fromWaypoint, char *toWaypoint);
ActionResult_enum PlanPointToPoint(Position_struct fromPoint, Position_struct toPoint);
ActionResult_enum PlanPointToWaypoint(Position_struct fromPoint, char *toWaypoint);

void FreePathPlan();

#ifdef __cplusplus
}
#endif

#endif
