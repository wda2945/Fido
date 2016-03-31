/*
 * luaglobals.c
 *
 *  Created on: Aug 13, 2014
 *      Author: martin
 */
//creates and updates robot-related global variables in the LUA environment

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include <dirent.h>
#include <string.h>
#include <math.h>
#include "lua.h"

#include "PubSubData.h"
#include "pubsub/pubsub.h"

#include "behavior/behavior.h"
#include "behavior/behaviorDebug.h"
#include "syslog/syslog.h"

static char *batteryStatusNames[] = BATTERY_STATUS_NAMES;

//load all (constant) lua globals with default values
int InitLuaGlobals(lua_State *L)
{
	int table, i;
	//initialize globals

	//settings and options
	lua_createtable(L, 0, 20);				//empty settings table
	lua_setglobal(L, "setting");

	lua_createtable(L, 0, 20);				//empty options table
	lua_setglobal(L, "option");

	lua_createtable(L, 0, 12);				//Empty environment table
	table = lua_gettop(L);

	lua_pushstring(L, "batteryVolts");
	lua_pushnumber(L, 0);
	lua_settable(L, table);

	lua_pushstring(L, "batteryAmps");
	lua_pushnumber(L, 0);
	lua_settable(L, table);

	lua_pushstring(L, "batteryAh");
	lua_pushnumber(L, 0);
	lua_settable(L, table);

	lua_pushstring(L, "solarVolts");
	lua_pushnumber(L, 0);
	lua_settable(L, table);

	lua_pushstring(L, "chargeVolts");
	lua_pushnumber(L, 0);
	lua_settable(L, table);

	lua_pushstring(L, "solarAmps");
	lua_pushnumber(L, 0);
	lua_settable(L, table);

	lua_pushstring(L, "chargeAmps");
	lua_pushnumber(L, 0);
	lua_settable(L, table);

	lua_pushstring(L, "internalTemp");
	lua_pushnumber(L, 0);
	lua_settable(L, table);

	lua_pushstring(L, "externalTemp");
	lua_pushnumber(L, 0);
	lua_settable(L, table);

	lua_pushstring(L, "relativeHumidity");
	lua_pushnumber(L, 0);
	lua_settable(L, table);

	lua_pushstring(L, "ambientLight");
	lua_pushnumber(L, 0);
	lua_settable(L, table);

	lua_pushstring(L, "isRaining");
	lua_pushboolean(L, 0);

	lua_settable(L, table);
	lua_setglobal(L, "environment");

	lua_createtable(L, 0, BATTERY_STATUS_COUNT + 2);					//Empty battery table

	lua_pushstring(L, "status");
	lua_pushinteger(L, BATTERY_UNKNOWN_STATUS);
	lua_settable(L, table);
	lua_pushstring(L, "volts");
	lua_pushnumber(L, 0);
	lua_settable(L, table);
	for (i=0; i< BATTERY_STATUS_COUNT; i++)
	{
		lua_pushstring(L, batteryStatusNames[i]);
		lua_pushinteger(L, i);
		lua_settable(L, table);
	}
	lua_setglobal(L, "battery");

	return 0;
}

//copy data into lua globals from relevant messages
int UpdateGlobalsFromMessage(lua_State *L, psMessage_t *msg)
{
	int table, i;
	switch (msg->header.messageType)
	{
	case BATTERY:
		lua_getglobal(L, "battery");
		table = lua_gettop(L);

		ASSERT_LUA_TABLE(L, table, "battery")

		lua_pushstring(L, "status");
		lua_pushinteger(L, msg->batteryPayload.status);
		lua_settable(L, table);

		lua_pushstring(L, "volts");
		lua_pushnumber(L, msg->batteryPayload.volts);
		lua_settable(L, table);

		break;
	case TICK_1S:
		//increment tick
		lua_getglobal(L, "secondCount");

		ASSERT_LUA_NUMBER(L, 1, "tick_1s")

		lua_pushinteger(L, lua_tointeger(L, 1) + 1);
		lua_setglobal(L, "secondCount");

		break;
	case SETTING:
	case NEW_SETTING:
		lua_getglobal(L, "setting");
		table = lua_gettop(L);

		ASSERT_LUA_TABLE(L, table, "setting")

		lua_pushstring(L, msg->settingPayload.name);
		lua_pushnumber(L, msg->settingPayload.value);
		lua_settable(L, table);
		break;
	case OPTION:
	case SET_OPTION:
		lua_getglobal(L, "option");
		table = lua_gettop(L);

		ASSERT_LUA_TABLE(L, table, "option")

		lua_pushstring(L, msg->optionPayload.name);
		lua_pushnumber(L, msg->optionPayload.value);
		lua_settable(L, table);
		break;

	default:
		//ignores other messages
		break;
	}
	lua_pop(L, lua_gettop( L));

	return 0;
}

