/*
 * waypoints.h
 *
 *  Created on: 2015
 *      Author: martin
 */

//Waypoint database

#ifndef WAYPOINTS_H_
#define WAYPOINTS_H_

#include "PubSubData.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void *next;
	char *waypointName;
	void *waypoint;
} WaypointList_struct;

typedef struct {
	void *next;
	char *waypointName;
	Position_struct position;
	WaypointList_struct *firstConnection, *lastConnection;
} Waypoint_struct;

extern Waypoint_struct *waypointListStart;
extern Waypoint_struct *waypointListEnd;

//lookup a waypoint
Waypoint_struct *GetWaypointByName(const char *name);

//maintaint wp database
int InitWaypointDB();
int AddWaypoint(const char *name, Position_struct position);
int DeleteWaypoint(const char *wp_name);
int AddWaypointConnection(const char *wp1, const char *wp2);

int LoadWaypointDatabase(const char *filename);
int SaveWaypointDatabase(const char *filename);

    
#ifdef __cplusplus
}
#endif

#endif
