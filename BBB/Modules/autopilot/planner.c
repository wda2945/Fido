/*
 ============================================================================
 Name        : planner.c
 Author      : Martin
 Version     :
 Copyright   : (c) 2016 Martin Lane-Smith
 Description : Route plan
 ============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>

#include "softwareprofile.h"

#include "planner.h"
#include "autopilot_common.h"
#include "behavior/behavior_enums.h"
#include "syslog/syslog.h"

//structs used by the path planner
//edge - node-to-node with cost
typedef struct _graphEdgeStruct {
	short to, from;
	double cost;
} GraphEdgeStruct;

//node - with a position and a set of edges
typedef struct _graphNodeStruct {
	short nodeIndex;
	short edgeCount;
	GraphEdgeStruct *edges;	//set of edge structs
	Position_struct position;
	char *nodeName;
	Waypoint_struct *waypoint;
//	CGRect rect;		//if a map region
} GraphNodeStruct;

int nodeCount = 0;
GraphNodeStruct *nodeList = NULL;

//search parameters
Position_struct sourcePoint, targetPoint;	//positions
int source, target;					//node numbers

//A* search stuff...

//indexed into by node #. Contains the cost from adding GCosts[n] to
//the heuristic cost from n to the target node. This is the vector the
//iPQ indexes into.
double           *FCosts = NULL;

//indexed into by node. Contains the 'real' accumulated cost to that node
double           *GCosts = NULL;

//this vector contains the edges that comprise the shortest path tree -
//a directed subtree of the graph that encapsulates the best paths from
//every node on the SPT to the source node.
GraphEdgeStruct			*shortestPathTree = NULL;

//this is an indexed (by node) vector of 'parent' edges leading to nodes
//connected to the SPT but that have not been added to the SPT yet. This is
//a little like the stack or queue used in BST and DST searches.
GraphEdgeStruct			*searchFrontier = NULL;

//IPQ - Indexed Priority Queue
//used to pick the most likely search frontier node next
int				*IPQ = NULL;
int				IPQCount = 0;

//private
ActionResult_enum NewWaypointPath();

int InitAstar();

int closestNodeTo(Position_struct posn, Position_struct inDirectionOfTarget);	//find a node on the graph

void doPathCycle();		//makes one iteration.

double HeuristicCalculate(int _from, int _to);	//A-Star heuristic

int getNodeIndex(char * nodeName);
double Vec2DistanceFt(Position_struct t, Position_struct s);

//priority queue
void pqInsert(int n);
int pqPop();
void pqChangePriority(int n);

//Public

ActionResult_enum PlanWPtoWP(char *fromWaypoint, char *toWaypoint)		//named point to named point
{
	source = getNodeIndex(fromWaypoint);
	if (source < 0)
	{
		lastLuaCallReason = "No FROM WP";
		return ACTION_FAIL;
	}
	DEBUGPRINT("Source %s is %i\n", fromWaypoint, source);
	target = getNodeIndex(toWaypoint);
	if (target < 0)
	{
		lastLuaCallReason = "No TO WP";
		return ACTION_FAIL;
	}
	DEBUGPRINT("Target %s is %i\n", toWaypoint, target);

	return NewWaypointPath();
}
ActionResult_enum PlanPointToPoint(Position_struct fromPoint, Position_struct toPoint)	//position to position
{
	source = closestNodeTo(fromPoint, toPoint);
	if (source < 0)
	{
		lastLuaCallReason = "No FROM WP";
		return ACTION_FAIL;
	}
	DEBUGPRINT("Closest Source %s is %i\n", nodeList[source].nodeName, source);

	target = closestNodeTo(toPoint, fromPoint);
	if (target < 0)
	{
		lastLuaCallReason = "No TO WP";
		return ACTION_FAIL;
	}
	DEBUGPRINT("Closest Target %s is %i\n", nodeList[target].nodeName, target);
	return NewWaypointPath();
}
ActionResult_enum PlanPointToWaypoint(Position_struct fromPoint, char *toWaypoint)		//position to named point
{
	source = closestNodeTo(fromPoint, nodeList[target].position);
	if (source < 0)
	{
		lastLuaCallReason = "No FROM WP";
		return ACTION_FAIL;
	}
	DEBUGPRINT("Closest Source %s is %i\n", nodeList[source].nodeName, source);

	target = getNodeIndex(toWaypoint);
	if (target < 0)
	{
		lastLuaCallReason = "No TO WP";
		return ACTION_FAIL;
	}
	DEBUGPRINT("Target %s is %i\n", toWaypoint, target);

	return NewWaypointPath();
}

ActionResult_enum NewWaypointPath()
{
	if (source < 0 || target < 0)
	{
		lastLuaCallReason = "Unreachable";
		return ACTION_FAIL;
	}

	DEBUGPRINT("Plan from %i to %i\n", source, target);

	InitAstar();

	//put the source node on the queue
	searchFrontier[source].to = source;
	FCosts[source] = GCosts[source] = 0.0f;
	pqInsert(source);

	bool searchComplete = false;

	while (1)
	{
		//uses an indexed priority queue of nodes. The nodes with the
		//lowest overall F cost (G+H) are positioned at the front.
		//while the queue is not empty
		//get lowest cost node from the queue
		int NextClosestNode = pqPop();

		//move this node/edge from the frontier to the spanning tree
		shortestPathTree[NextClosestNode] = searchFrontier[NextClosestNode];
		DEBUGPRINT("NextNode = %i (from %i)\n", NextClosestNode, shortestPathTree[NextClosestNode].from);
		//if the target has been found exit
		if (NextClosestNode == target ) {
			searchComplete = true;
			DEBUGPRINT("A* Search complete\n");
			break;
		}
		else
			//if we've failed exit
			if (NextClosestNode < 0) {
				searchComplete = false;
				DEBUGPRINT("A* Search failed\n");
				break;
			}
			else
			{
				//now to test all the edges attached to this node
				for (int i = 0; i < nodeList[NextClosestNode].edgeCount; i++)
				{
					int newNode = nodeList[NextClosestNode].edges[i].to;

					//calculate the heuristic cost from this node to the target (H)
					double HCost = HeuristicCalculate(target, newNode);

					//calculate the 'real' cost to this node from the source (G)
					double GCost = GCosts[NextClosestNode] + nodeList[NextClosestNode].edges[i].cost;

					//if the node has not been added to the frontier, add it and update
					//the G and F costs
					if (searchFrontier[newNode].to == -1)
					{
						FCosts[newNode] = GCost + HCost;
						GCosts[newNode] = GCost;

						pqInsert(newNode);

						searchFrontier[newNode] = nodeList[NextClosestNode].edges[i];
					}

					//if this node is already on the frontier but the cost to get here
					//is cheaper than has been found previously, update the node
					//costs and frontier accordingly.
					else if ((GCost < GCosts[newNode]) && (shortestPathTree[newNode].to == -1))
					{
						FCosts[newNode] = GCost + HCost;
						GCosts[newNode] = GCost;

						pqChangePriority(newNode);

						searchFrontier[newNode] = nodeList[NextClosestNode].edges[i];
					}
				}
			}
	}
	
	if (searchComplete)
	{
		//count path steps
		int nd = target;
		int pathCount = 1;

		while ((nd != source) && (shortestPathTree[nd].to != -1))
		{
			pathCount++;
			nd = shortestPathTree[nd].from;
		}
		planWaypointCount = pathCount;
		planWaypoints = calloc(pathCount, sizeof(char*));
		totalPlanCost = 0.0;

		if (!planWaypoints)
		{
			DEBUGPRINT("A* No memory for PathPlan names\n");
			return ACTION_FAIL;
		}

		nd = target;

		while ((nd != source) && (shortestPathTree[nd].to != -1))
		{
			planWaypoints[--pathCount] = nodeList[nd].nodeName;
			totalPlanCost += shortestPathTree[nd].cost;

			nd = shortestPathTree[nd].from;
		}
		planWaypoints[0] = nodeList[source].nodeName;

		LogRoutine("A* %i steps, total cost %.1f\n", planWaypointCount, totalPlanCost);

		for (int i=0; i<planWaypointCount; i++)
		{
			LogRoutine("%2i: %s\n", i+1, planWaypoints[i]);
		}

		routeWaypointIndex 		= 0;
		nextWaypointName 	= planWaypoints[0];
		nextPosition		= nodeList[0].position;

		return ACTION_SUCCESS;
	}
	else
	{
		lastLuaCallReason = "A* Fail";
		return ACTION_FAIL;
	}
}



//release all memory
void FreePathPlan()
{
	char **wpName = planWaypoints;
	
	while(*wpName)
	{
		free(*wpName);
		wpName++;
	}
}

//Private

int InitNodeList()
{
	int i;
	//convert from a linked list to an array
	//free old one, if any
	if (nodeList)
	{
		for (i=0; i< nodeCount; i++)
		{
			//free edge structs
			free(nodeList[i].edges);
		}
		free(nodeList);
	}

	nodeCount = 0;

	//count the nodes
	Waypoint_struct *current = waypointListStart;
	while (current != NULL)
	{
		nodeCount++;
		current = current->next;
	}
	if (nodeCount)
	{
		nodeList = calloc(nodeCount, sizeof(GraphNodeStruct));
		if (!nodeList)
		{
			ERRORPRINT("No memory for nodeList\n");
			return -1;
		}
		//init the structs
		current = waypointListStart;
		i = 0;
		while (current != NULL)
		{
			nodeList[i].nodeIndex = i;
			nodeList[i].position = current->position;
			nodeList[i].nodeName = current->waypointName;
			nodeList[i].waypoint = current;
			//do edges in second pass
			i++;
			current = current->next;
		}
		//go through again and init edges
		for (i=0; i< nodeCount; i++)
		{
			//count edges
			WaypointList_struct *conn = nodeList[i].waypoint->firstConnection;
			while (conn)
			{
				nodeList[i].edgeCount++;
				conn = conn->next;
			}
			if (nodeList[i].edgeCount)
			{
				nodeList[i].edges = calloc(nodeList[i].edgeCount, sizeof(GraphEdgeStruct));
				if (!nodeList[i].edges)
				{
					ERRORPRINT("No memory for edge structs\n");
					return -1;
				}
				//init edge structs
				int e = 0;
				conn = nodeList[i].waypoint->firstConnection;
				while (conn)
				{
					nodeList[i].edges[e].from = i;

					int toNode = getNodeIndex(conn->waypointName);

					if (toNode >= 0)
					{
						//to found
						nodeList[i].edges[e].to = toNode;
						nodeList[i].edges[e].cost = Vec2DistanceFt(nodeList[i].position, nodeList[toNode].position);

						DEBUGPRINT("Edge from %s (%i) to %s (%i)\n", nodeList[i].nodeName, i, nodeList[toNode].nodeName, toNode);
					}
					else
					{
						LogError("A* toNode %s not found\n", conn->waypointName);
						return -1;
					}
					conn = conn->next;
					e++;
				}
			}
		}
	}
	return 0;
}

int InitAstar()
{
	if (FCosts) free(FCosts);
	if (GCosts) free(GCosts);
	if (shortestPathTree) free(shortestPathTree);
	if (searchFrontier) free(searchFrontier);
	if (IPQ) free(IPQ);
    FCosts				= calloc(nodeCount, sizeof(double));
    GCosts				= calloc(nodeCount, sizeof(double));
    shortestPathTree	= calloc(nodeCount, sizeof(GraphEdgeStruct));
    searchFrontier		= calloc(nodeCount, sizeof(GraphEdgeStruct));
    IPQ					= calloc(nodeCount, sizeof(int));
    IPQCount = 0;

	for (int i = 0; i < nodeCount; i++){
		searchFrontier[i].to = -1;
		shortestPathTree[i].to = -1;
	}

	return 0;
}

int closestNodeTo(Position_struct posn, Position_struct inDirectionOfTarget)
{
	double bestRange = 1000000.0;
	int node = -1;

	//find a node on the graph
	for (int i=0; i< nodeCount; i++)
	{
		double range = Vec2DistanceFt(posn, nodeList[i].position) + Vec2DistanceFt(inDirectionOfTarget, nodeList[i].position);
		if (range < bestRange)
		{
			bestRange = range;
			node = i;
		}
	}
	return node;
}

double HeuristicCalculate(int _from, int _to)
{
	//A-Star heuristic
	return Vec2DistanceFt(nodeList[_from].position, nodeList[_to].position);
}

int getNodeIndex(char * nodeName)
{
	int j;
	for (j=0; j<nodeCount; j++)
	{
		if (strcmp(nodeName, nodeList[j].nodeName) == 0)
		{
			return j;
			break;
		}
	}
	return -1;
}

double Vec2DistanceFt(Position_struct t, Position_struct s)
{
	return sqrt((t.latitude - s.latitude) * (t.latitude - s.latitude) + (t.longitude - s.longitude) * (t.longitude - s.longitude)) * 60 * 5280;
}

//priority queue
void pqInsert(int n)
{
	IPQ[IPQCount++] = n;

	DEBUGPRINT("IPQ inserted %i at %i\n", n, IPQCount);
}
int pqPop()
{
	int minCost = 100000;
	int index = -1;
	int result = -1;

	for (int i = 0; i < IPQCount; i++){
		if ( minCost > FCosts[IPQ[i]]) {
			minCost = FCosts[IPQ[i]];
			index = i;
		}
	}

	if (index >= 0){
		result = IPQ[index];
		for (int i = index + 1; i < IPQCount; i++){
			IPQ[i - 1] = IPQ[i];
		}
		IPQCount--;
	}

	DEBUGPRINT("IPQ pop = %i, IPQCount = %i\n", result, IPQCount);

	return result;
}
void pqChangePriority(int n)
{
	//null because priority handled during pop
}


