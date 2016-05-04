/*
 * ScriptPlayer.c
 *
 *  Created on: Aug 10, 2014
 *      Author: martin
 */
// Controller of the LUA subsystem

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "lua.h"
#include "lualib.h"

#include "pubsubdata.h"
#include "pubsub/pubsub.h"

#include "behavior/behavior.h"
#include "behavior/behavior_enums.h"
#include "behavior/behaviordebug.h"
#include "autopilot/autopilot.h"
#include "syslog/syslog.h"

lua_State	*btLuaState = NULL;

//logging
static int Print(lua_State *L);				//Print("...")
static int ErrorPrint(lua_State *L);				//Print("...")
static int Alert(lua_State *L);
static int Fail(lua_State *L);

char behaviorName[PS_NAME_LENGTH] = "Idle";
BehaviorStatus_enum behaviorStatus = 0;

char lastActivityName[PS_NAME_LENGTH] = "none";
char lastActivityStatus[PS_NAME_LENGTH] = "invalid";

char *lastLuaCall 			= "";
char *lastLuaCallReason 	= "";
char *lastLuaCallFail 		= "";

//allocator for lua state
static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize)
{
	(void)ud;  (void)osize;  /* not used */
	if (nsize == 0)
	{
		free(ptr);
		return NULL;
	}
	else
		return realloc(ptr, nsize);
}

int InitScriptingSystem()
{
	lua_State	*newLuaState;

	//create a new LUA state
	if (btLuaState)
	{
		//close if previously opened
		lua_close(btLuaState);
		btLuaState = NULL;
	}

	newLuaState = lua_newstate(l_alloc, NULL);

	if (newLuaState == NULL)
	{
		ERRORPRINT("luaState create fail\n");
	   	return -1;
	}

	//open standard libraries
	luaL_openlibs(newLuaState);

	luaopen_math(newLuaState);
	lua_setglobal(newLuaState, "math");
	luaopen_string(newLuaState);
	lua_setglobal(newLuaState, "string");

	//register basic call-backs
	lua_pushcfunction(newLuaState, Alert);				//Alert Message to App
	lua_setglobal(newLuaState, "Alert");
	lua_pushcfunction(newLuaState, Print);				//Print Log Message
	lua_setglobal(newLuaState, "Print");
	lua_pushcfunction(newLuaState, ErrorPrint);			//Print Error Message
	lua_setglobal(newLuaState, "ErrorPrint");
	lua_pushcfunction(newLuaState, Fail);				//Note fail() context - Fail('name')
	lua_setglobal(newLuaState, "Fail");

	int reply = 0;
	reply += InitPilotingCallbacks(newLuaState);			//set BT-related call-backs
	reply += InitProximityCallbacks(newLuaState);
	reply += InitSystemCallbacks(newLuaState);
	reply += InitLuaGlobals(newLuaState);				//load system global constants
	reply += LoadAllScripts(newLuaState);				//load all LUA scripts

	lua_pop(newLuaState, lua_gettop( newLuaState));		//clean stack

	//make initialized state available to other threads
	btLuaState = newLuaState;

	if (reply == 0) return 0;
	else return -1;
}

//messages processed by scripting system
int ScriptProcessMessage(psMessage_t *msg)
{
	if (!btLuaState) return -1;

	switch (msg->header.messageType)
	{
	case RELOAD:
		//reload all scripts
		DEBUGPRINT("Reload scripts\n");
		if (InitScriptingSystem() < 0)
		{
			LogError("Error on Reload scripts\n");
			ERRORPRINT("Error on Reload scripts\n");
		}
		break;

	case ACTIVATE:
		//start BT activity
		DEBUGPRINT("Activate: %s\n", msg->namePayload.name);

		lua_getglobal(btLuaState, "activate");						//reference to 'activate(...)
		if (lua_isfunction(btLuaState, -1))
		{
			//activate() exists
			lua_pushstring(btLuaState, msg->namePayload.name);		//name of target BT
			lua_getglobal(btLuaState, msg->namePayload.name);		//reference to the tree itself
			if (lua_istable(btLuaState, -1))
			{
				//target table exists - call activate()
				strncpy(behaviorName, msg->namePayload.name, PS_NAME_LENGTH);

				int status = lua_pcall(btLuaState, 2, 0, 0);
				if (status != 0)
				{
					const char *errormsg = lua_tostring(btLuaState, -1);
					ERRORPRINT("%s, Activate error: %s\n",msg->namePayload.name,errormsg);
					behaviorStatus = BEHAVIOR_INVALID;
				}
				else
				{
					behaviorStatus = BEHAVIOR_ACTIVE;
				}
			}
			else
			{
				//target BT does not exist
				LogError("%s is type %s\n",msg->namePayload.name,lua_typename(btLuaState,lua_type(btLuaState, -1)) );
				ERRORPRINT("%s is type %s\n",msg->namePayload.name,lua_typename(btLuaState,lua_type(btLuaState, -1)) );
			}
		}
		else
		{
			//activate() does not exist
			LogError("activate is type %s\n",lua_typename(btLuaState,lua_type(btLuaState, -1)) );
			ERRORPRINT("activate is type %s\n",lua_typename(btLuaState,lua_type(btLuaState, -1)) );
		}
		lua_pop(btLuaState, lua_gettop( btLuaState));
		break;

	default:
		//process others to update globals
		return UpdateGlobalsFromMessage(btLuaState, msg);
	}
	return 0;
}

static char *behaviorStatusNames[BEHAVIOR_STATUS_COUNT] = BEHAVIOR_STATUS_NAMES;

//periodic behavior tree update invocation
int InvokeUpdate()
{
	if (!btLuaState) return -1;

	//prepare Activity msg
	psMessage_t activityMsg;
	psInitPublish(activityMsg, ACTIVITY);
	strncpy(activityMsg.behaviorStatusPayload.behavior, behaviorName, PS_SHORT_NAME_LENGTH);

	switch (behaviorStatus)
	{
	case BEHAVIOR_ACTIVE:
	case BEHAVIOR_RUNNING:

		lua_getglobal(btLuaState, "update");

		if (lua_isfunction(btLuaState, -1))
		{
			//		DEBUGPRINT("LUA: calling update\n");
			int reply = lua_pcall(btLuaState, 0, 1, 0);
			if (reply)
			{
				const char *errormsg = lua_tostring(btLuaState, -1);
				ERRORPRINT("Script update, Error: %s\n",errormsg);
				lua_pop(btLuaState, lua_gettop( btLuaState));
				behaviorStatus = BEHAVIOR_ERROR;
				strncpy(activityMsg.behaviorStatusPayload.lastLuaCallFail, "update", PS_SHORT_NAME_LENGTH);
				strncpy(activityMsg.behaviorStatusPayload.lastFailReason, "LUA Error", PS_SHORT_NAME_LENGTH);
				activityMsg.behaviorStatusPayload.status = behaviorStatus;
				RouteMessage(&activityMsg);

				return -1;
			}
			else
			{
				size_t len;
				const char *status = lua_tolstring(btLuaState,1, &len);

				DEBUGPRINT("LUA: returned from update (%s)\n", status);

				BehaviorStatus_enum statusCode = BEHAVIOR_INVALID;
				for (int i=2; i<BEHAVIOR_STATUS_COUNT; i++)
				{
					if (strncmp(behaviorStatusNames[i], status, 4) == 0)
					{
						statusCode = i;
						break;
					}
				}

				if ((statusCode != behaviorStatus) || (strcmp(lastActivityName, behaviorName) != 0))
				{
					//change in activity or status
					activityMsg.behaviorStatusPayload.status = statusCode;

					switch (statusCode)
					{
					case BEHAVIOR_SUCCESS:
						DEBUGPRINT("%s SUCCESS\n", behaviorName);
						activityMsg.behaviorStatusPayload.lastLuaCallFail[0] = '\0';
						activityMsg.behaviorStatusPayload.lastFailReason[0] = '\0';
						RouteMessage(&activityMsg);
						break;
					case BEHAVIOR_RUNNING:
						activityMsg.behaviorStatusPayload.lastLuaCallFail[0] = '\0';
						activityMsg.behaviorStatusPayload.lastFailReason[0] = '\0';
						RouteMessage(&activityMsg);
						break;
					case BEHAVIOR_FAIL:
						DEBUGPRINT("%s FAIL @ %s - %s\n", behaviorName, lastLuaCallFail, lastLuaCallReason);
						strncpy(activityMsg.behaviorStatusPayload.lastLuaCallFail, lastLuaCallFail, PS_SHORT_NAME_LENGTH);
						strncpy(activityMsg.behaviorStatusPayload.lastFailReason, lastLuaCallReason, PS_SHORT_NAME_LENGTH);
						RouteMessage(&activityMsg);
						break;
					case BEHAVIOR_ACTIVE:
					case BEHAVIOR_INVALID:
					default:
						DEBUGPRINT("%s Bad Response\n", behaviorName);
						strncpy(activityMsg.behaviorStatusPayload.lastLuaCallFail, "update", PS_SHORT_NAME_LENGTH);
						strncpy(activityMsg.behaviorStatusPayload.lastFailReason, "Bad Status", PS_SHORT_NAME_LENGTH);
						RouteMessage(&activityMsg);
						break;
					}

//					DEBUGPRINT("Activity %s, status: %s\n", behaviorName, status);
					behaviorStatus = statusCode;
				}

				lua_pop(btLuaState, lua_gettop( btLuaState));

				strncpy(lastActivityName, behaviorName, PS_NAME_LENGTH);
				return 0;
			}
		}
		else
		{
			LogError("Global update is of type %s\n",lua_typename(btLuaState,lua_type(btLuaState, -1)) );
			ERRORPRINT("Global update is of type %s\n",lua_typename(btLuaState,lua_type(btLuaState, -1)) );
			lua_pop(btLuaState, lua_gettop( btLuaState));
			return -1;
		}
		break;
	default:
		break;
	}
}

//report available activity scripts
int AvailableScripts()
{
	if (!btLuaState) return 0;	//not yet initialized

	DEBUGPRINT("Report Available Activities\n");

	struct timespec requested_time, remaining;
	psMessage_t msg;
	int i;
	int messageCount = 0;

	psInitPublish(msg, SCRIPT);

	//look up activities table
	lua_getglobal(btLuaState, "ActivityList");
	int table = lua_gettop( btLuaState);

	if (lua_istable(btLuaState, table) == 0)
	{
		//not a table
		LogError("No Activities table\n" );
		ERRORPRINT("No Activities table\n" );
		lua_pop(btLuaState, lua_gettop( btLuaState));
		return 0;
	}

    lua_pushnil(btLuaState);  /* first key */
    while (lua_next(btLuaState, table) != 0) {
      /* uses 'key' (at index -2) and 'value' (at index -1) */
		strncpy(msg.namePayload.name, lua_tostring(btLuaState, -1), PS_NAME_LENGTH);
		DEBUGPRINT("Activity: %s\n", msg.namePayload.name);

		RouteMessage(&msg);
		messageCount++;

		//delay
		usleep(MESSAGE_DELAY * 1000);

      /* removes 'value'; keeps 'key' for next iteration */
      lua_pop(btLuaState, 1);
    }
	lua_pop(btLuaState, lua_gettop( btLuaState));

	DEBUGPRINT("%i activities reported\n", messageCount);

	return messageCount;
}

//call-backs

//Alert
static int Alert(lua_State *L)				//Alert("...")
{
	const char *text = lua_tostring(L,1);

	psMessage_t msg;
	psInitPublish(msg, ALERT);
	strncpy(msg.namePayload.name, text, PS_NAME_LENGTH);
	RouteMessage(&msg);
	DEBUGPRINT("lua: Alert (%s)\n", text);

	return 0;
}

//print
static int Print(lua_State *L)				//Print("...")
{
	const char *text = lua_tostring(L,1);
	DEBUGPRINT("lua: %s\n",text);
	return 0;
}

static int ErrorPrint(lua_State *L)				//Print("...")
{
	const char *text = lua_tostring(L,1);
	ERRORPRINT("lua: %s\n",text);
	return 0;
}
//Fail
char failBuffer[PS_NAME_LENGTH];
static int Fail(lua_State *L)				//Fail('name')
{
	const char *name = lua_tostring(L,1);

	if (name)
	{
		strncpy(failBuffer, name, PS_NAME_LENGTH);
		lastLuaCallFail = failBuffer;
	}

	DEBUGPRINT("lua: fail at %s\n",name);
	return 0;
}

