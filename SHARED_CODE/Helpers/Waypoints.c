/*
 ============================================================================
 Name        : waypoints.c
 Author      : Martin
 Version     :
 Copyright   : (c) 2015 Martin Lane-Smith
 Description : Load and maintain waypoint list
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

#include "mxml.h"
#include "waypoints.h"

Waypoint_struct *waypointListStart = NULL;
Waypoint_struct *waypointListEnd = NULL;

int InitWaypointDB()
{
	while (waypointListStart)
	{
		DeleteWaypoint(waypointListStart->waypointName);
	}
    return 0;
}

int AddWaypoint(const char *name, Position_struct _position)
{
	//make the wp struct
	Waypoint_struct *wp = (Waypoint_struct*) calloc(sizeof(Waypoint_struct), 1);
	if (wp == NULL) return -1;	//no memory

	wp->waypointName = calloc(strlen(name)+1, 1);
	if (wp->waypointName == NULL)
	{
		free(wp);
		return -1;	//no memory
	}
    strcpy(wp->waypointName, name);
    wp->position = _position;
    
	if (waypointListStart == NULL)
	{
		//list is empty
		waypointListStart = waypointListEnd = wp;
		wp->next = NULL;
	}
	else
	{
		Waypoint_struct *current = waypointListStart;
		Waypoint_struct *prior = NULL;

		while (current != NULL)
		{
			int comp = strcmp(wp->waypointName, current->waypointName);

			if (comp == 0)
			{
				//update position
				current->position = _position;

				//free the unneeded new one
				free(wp->waypointName);
				free(wp);
				return -1; //duplicate
			}

			if (comp < 0)
			{
				//insert before current
				wp->next = current;
				if (prior)
				{
					prior->next = wp;
				}
				else
				{
					//going before first entry
					waypointListStart = wp;
				}
				return 0;
			}
			else
			{
				//step down list
				prior = current;
				current = current->next;
			}
		}
		//ran off bottom
		prior->next = wp;
		wp->next = NULL;
		waypointListEnd = wp;
	}
	return 0;
}


Waypoint_struct *GetWaypointByName(const char *name)
{
	if (waypointListStart == NULL)
	{
		//list is empty
		return (Waypoint_struct *) 0;
	}
	else
	{
		Waypoint_struct *current = waypointListStart;
		Waypoint_struct *prior = NULL;

		while (current != NULL)
		{
			int comp = strcmp(name, current->waypointName);

			if (comp == 0)
			{
				return current;
			}

			if (comp < 0)
			{
				//not found
				return (Waypoint_struct *) 0;
			}
			else
			{
				//step down list
				prior = current;
				current = current->next;
			}
		}
		//ran off bottom
	}
	return (Waypoint_struct *) 0;
}

int DeleteWaypoint(const char *wp_name)
{
	if (waypointListStart == NULL) return -1;	//empty list

	Waypoint_struct *current = waypointListStart;
	Waypoint_struct *prior = NULL;

	while (current != NULL)
	{
		int comp = strcmp(wp_name, current->waypointName);

		if (comp == 0)
		{
			if (prior)
			{
				prior->next = current->next;
				if (current->next == NULL) waypointListEnd = prior;
			}
			else
			{
				waypointListStart = current->next;
				if (waypointListStart == NULL) waypointListEnd = NULL; //last entry
			}
			free(current);
			return 0;
		}
		else
		{
			//step down list
			prior = current;
			current = current->next;
		}
	}
	return -1;	//not found
}

int InsertWaypointConnection(const char *wp_name, const char *target_name)
{
	Waypoint_struct *wp = GetWaypointByName(wp_name);

	if (wp == NULL) return -1;	//not found

	WaypointList_struct *wp_list = calloc(sizeof(WaypointList_struct), 1);

	if (wp_list == NULL) return -1;	//no memory

	wp_list->waypointName = calloc(strlen(target_name)+1, 1);
	if (wp_list->waypointName == NULL)
	{
		free(wp_list);
		return -1;	//no memory
	}
	strcpy(wp_list->waypointName, target_name);

	if (wp->lastConnection == NULL)
	{
		wp->lastConnection = wp->firstConnection = wp_list;
	}
	else
	{
		wp->lastConnection->next = wp_list;
		wp_list->next = NULL;
		wp->lastConnection = wp_list;
	}
	return 0;
}

int AddWaypointConnection(const char *wp1, const char *wp2)
{
	if (InsertWaypointConnection(wp1, wp2) < 0) return -1;
	if (InsertWaypointConnection(wp2, wp1) < 0) return -1;
	return 0;
}

mxml_type_t type_cb(mxml_node_t *node)
{
    const char *type;
    
    type = mxmlElementGetAttr(node, "type");
    if (type == NULL)
        type = mxmlGetElement(node);
    
    if (!strcmp(type, "real"))
        return (MXML_REAL);
    else
        return (MXML_TEXT);
}


int LoadWaypointDatabase(const char *filename)
{
    FILE *fp;
    mxml_node_t *xml;    /* <?xml ... ?> */

    mxml_node_t *waypoint;   /* <waypoint> */
    mxml_node_t *name;  /* <name> */
    mxml_node_t *latitude;  /* <latitude> */
    mxml_node_t *longitude;  /* <longitude> */

    mxml_node_t *connection;  /* <connection> */
    mxml_node_t *cName;

    const char *waypointName;
    Position_struct position;

    fp = fopen(filename, "r");

    if (fp)
    {
    	InitWaypointDB();

    	xml = mxmlLoadFile(NULL, fp, type_cb);
    	fclose(fp);

    	//walk tree
    	//iterate waypoints
        for (waypoint = mxmlFindElement(xml, xml,
                                    "waypoint",
                                    NULL, NULL,
                                    MXML_DESCEND);
        		waypoint != NULL;
        		waypoint = mxmlFindElement(waypoint, xml,
                                    "waypoint",
                                    NULL, NULL,
                                    MXML_DESCEND))
        {
        	name = mxmlFindElement(waypoint, waypoint,
        	                                    "name",
        	                                    NULL, NULL,
        	                                    MXML_DESCEND);
        	waypointName = mxmlGetText(name, NULL);

        	latitude = mxmlFindElement(waypoint, waypoint,
        	                                    "latitude",
        	                                    NULL, NULL,
        	                                    MXML_DESCEND);
        	position.latitude = mxmlGetReal(latitude);

        	longitude = mxmlFindElement(waypoint, waypoint,
            	                                    "longitude",
            	                                    NULL, NULL,
            	                                    MXML_DESCEND);
            position.longitude = mxmlGetReal(longitude);

            if (waypointName)
            {
            	AddWaypoint(waypointName, position);
            	printf("WPXML: %s\n", waypointName);

            	//iterate connections
                for (connection = mxmlFindElement(waypoint, waypoint,
                                            "connection",
                                            NULL, NULL,
                                            MXML_DESCEND);
                		connection != NULL;
                		connection = mxmlFindElement(connection, waypoint,
                                            "connection",
                                            NULL, NULL,
                                            MXML_DESCEND))
                {
                	cName = mxmlFindElement(connection, connection,
                	                                    "name",
                	                                    NULL, NULL,
                	                                    MXML_DESCEND);
                	const char *connectionName = mxmlGetText(cName, NULL);

                	if (connectionName)
                	{
                		InsertWaypointConnection(waypointName, connectionName);
                       	printf("WPXML: %s -> %s\n", waypointName, connectionName);

                    }
                	else
                	{
                		printf("WPXML: Missing connectionName in '%s'\n", waypointName);
                	}
                }
            }
            else
            {
        		printf("WPXML: Missing waypointName\n");
            }
        }
    	mxmlDelete(xml);
    }
    else
    {
		printf("WPXML: can't open %s for read\n", filename);
        return -1;
    }
    return 0;
}

int SaveWaypointDatabase(const char *filename)
{
    mxml_node_t *xml;    /* <?xml ... ?> */
    mxml_node_t *data;   /* <data> */

    mxml_node_t *waypoint;   /* <waypoint> */
    mxml_node_t *name;      /* <name> */
    mxml_node_t *latitude;  /* <latitude> */
    mxml_node_t *longitude;  /* <longitude> */

    mxml_node_t *connections;  /* <connections> */
    mxml_node_t *connection;  /* <connection> */

    xml = mxmlNewXML("1.0");

    data = mxmlNewElement(xml, "data");

	Waypoint_struct *current = waypointListStart;

	while (current != NULL)
	{
		waypoint = mxmlNewElement(data, "waypoint");

		name = mxmlNewElement(waypoint, "name");
        mxmlElementSetAttr (name, "type", "text");
        mxmlNewText(name, 0, current->waypointName);

        latitude = mxmlNewElement(waypoint, "latitude");
        mxmlElementSetAttr (latitude, "type", "real");
        mxmlNewReal(latitude, current->position.latitude);

        longitude = mxmlNewElement(waypoint, "longitude");
        mxmlElementSetAttr (longitude, "type", "real");
        mxmlNewReal(longitude, current->position.longitude);

        WaypointList_struct *currentConnection = current->firstConnection;
        connections = mxmlNewElement(waypoint, "connections");

        while (currentConnection != NULL)
        {
        	connection = mxmlNewElement(connections, "connection");

    		name = mxmlNewElement(connection, "name");
            mxmlElementSetAttr (name, "type", "text");
            mxmlNewText(name, 0, currentConnection->waypointName);

            currentConnection = currentConnection->next;
        }

		//step down list
		current = current->next;
	}

    FILE *fp;

    fp = fopen(filename, "w");

    if (fp)
    {
    	mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);
    	fclose(fp);
    }
    else
    {
		printf("WPXML: can't open %s for write\n", filename);
        return -1;
    }
    mxmlDelete(xml);
    return 0;
}
