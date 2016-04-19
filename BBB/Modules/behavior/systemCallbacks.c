/*
 * BT Callbacks.c
 *
 *  Created on: Aug 10, 2014
 *      Author: martin
 */
// BT Leaf callbacks for lua
// System Callbacks

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>

#include "lua.h"
#include "lualib.h"

#include "pubsubdata.h"
#include "pubsub/pubsub.h"
#include "pubsub/notifications.h"

#include "behavior/behavior_enums.h"
#include "behavior/behavior.h"
#include "behavior/behaviordebug.h"
#include "syslog/syslog.h"

//actual leaf node
static int SystemAction(lua_State *L);

int SaveSettingsAndOptions(lua_State *L, bool useDefault);
int LoadSettingsAndOptions(lua_State *L);

typedef enum {
	SaveSettings,
	LoadSettings,
	ResetSavedSettings,
	NewWakeMask,
	WakeIn1Hour,
	WakeOnBumper,
	WakeOnIR,
	WakeOnPIR,
	WakeOnBatteryLow,
	WakeOnBatteryCritical,
	WakeOnChargingComplete,
	WakeAtSunset,
	WakeAtSunrise,
	WakeOnAppOnline,
    SystemPoweroff,
    SystemSleep,
    SystemWakeOnEvent,
    SystemSetResting,
    SystemSetActive,
    ReloadScripts,
    StrobeLightsOn,
    StrobeLightsOff,
    FrontLightsOn,
    FrontLightsOff,
    RearLightsOn,
    RearLightsOff,
    NavLightsOn,
    NavLightsOff,
    isCharging,
    isChargeComplete,
    isAppOnline,
    isRaining,
	SYSTEM_ACTION_COUNT
} SystemAction_enum;

static char *systemActionList[] = {
		"SaveSettings",
		"LoadSettings",
		"ResetSavedSettings",
		"NewWakeMask",
		"WakeIn1Hour",
		"WakeOnBumper",
		"WakeOnIR",
		"WakeOnPIR",
		"WakeOnBatteryLow",
		"WakeOnBatteryCritical",
		"WakeOnChargingComplete",
		"WakeAtSunset",
		"WakeAtSunrise",
		"WakeOnAppOnline",
	    "SystemPoweroff",
	    "SystemSleep",
	    "SystemWakeOnEvent",
	    "SystemSetResting",
	    "SystemSetActive",
	    "ReloadScripts",
	    "StrobeLightsOn",
	    "StrobeLightsOff",
	    "FrontLightsOn",
	    "FrontLightsOff",
	    "RearLightsOn",
	    "RearLightsOff",
	    "NavLightsOn",
	    "NavLightsOff",
	    "isCharging",
	    "isChargeComplete",
	    "isAppOnline",
	    "isRaining"
};

static NotificationMask_t WakeMask;

int InitSystemCallbacks(lua_State *L)
{
	int i, table;
	lua_pushcfunction(L, SystemAction);
	lua_setglobal(L, "SystemAction");

	lua_createtable(L, 0, SYSTEM_ACTION_COUNT);
	table = lua_gettop(L);
	for (i=0; i< SYSTEM_ACTION_COUNT; i++)
	{
		lua_pushstring(L, systemActionList[i]);
		lua_pushinteger(L, i);
		lua_settable(L, table);
	}
	lua_setglobal(L, "system");

	return 0;
}

static int SystemAction(lua_State *L)
{
	psMessage_t msg;

	SystemAction_enum actionCode 	= lua_tointeger(L, 1);

	lastLuaCall = systemActionList[actionCode];	//for error reporting

	DEBUGPRINT("System Action: %s\n", lastLuaCall);

	switch (actionCode)
	{
	case SaveSettings:
		return SaveSettingsAndOptions(L, false);
		break;
	case LoadSettings:
		return LoadSettingsAndOptions(L);
		break;
	case ResetSavedSettings:
		return SaveSettingsAndOptions(L, true);
		break;
	case NewWakeMask:
		WakeMask = 0;
		return success(L);
		break;
	case WakeIn1Hour:
		WakeMask |= NOTIFICATION_MASK(ONE_HOUR_EVENT);
		return success(L);
		break;
	case WakeOnBumper:
		WakeMask |= NOTIFICATION_MASK(FRONT_CONTACT_EVENT) | NOTIFICATION_MASK(REAR_CONTACT_EVENT);
		return success(L);
		break;
	case WakeOnIR:
		WakeMask |= NOTIFICATION_MASK(PROXIMITY_EVENT);
		return success(L);
		break;
	case WakeOnPIR:
		WakeMask |= NOTIFICATION_MASK(PIR_EVENT);
		return success(L);
		break;
	case WakeOnBatteryLow:
		WakeMask |= NOTIFICATION_MASK(BATTERY_LOW_EVENT);
		return success(L);
		break;
	case WakeOnBatteryCritical:
		WakeMask |= NOTIFICATION_MASK(BATTERY_CRITICAL_EVENT);
		return success(L);
		break;
	case WakeOnChargingComplete:
		WakeMask |= NOTIFICATION_MASK(CHARGING_COMPLETE_EVENT);
		return success(L);
		break;
	case WakeAtSunset:
		WakeMask |= NOTIFICATION_MASK(SUNSET_EVENT);
		return success(L);
		break;
	case WakeAtSunrise:
		WakeMask |= NOTIFICATION_MASK(SUNRISE_EVENT);
		return success(L);
		break;
	case WakeOnAppOnline:
		WakeMask |= NOTIFICATION_MASK(APPONLINE_EVENT);
		return success(L);
		break;
	case SystemPoweroff:
		psInitPublish(msg, OVM_COMMAND);
		msg.bytePayload.value = OVERMIND_POWEROFF;
		RouteMessage(&msg);
		return success(L);
		break;
	case SystemSleep:
		psInitPublish(msg, OVM_COMMAND);
		msg.bytePayload.value = OVERMIND_SLEEP;
		RouteMessage(&msg);
		return success(L);
		break;
	case SystemWakeOnEvent:
		psInitPublish(msg, WAKE_MASK);		//first send the wake mask
		msg.maskPayload.value[0] = WakeMask;
		RouteMessage(&msg);
		psInitPublish(msg, OVM_COMMAND);	//then sleep
		msg.bytePayload.value = OVERMIND_WAKE_ON_EVENT;
		RouteMessage(&msg);
		return success(L);
		break;
	case SystemSetResting:
		psInitPublish(msg, OVM_COMMAND);
		msg.bytePayload.value = OVERMIND_REQUEST_ACTIVE;
		RouteMessage(&msg);
		return success(L);
		break;
	case SystemSetActive:
		psInitPublish(msg, OVM_COMMAND);
		msg.bytePayload.value = OVERMIND_REQUEST_ACTIVE;
		RouteMessage(&msg);
		return success(L);
		break;
	case ReloadScripts:
		psInitPublish(msg, RELOAD);
		RouteMessage(&msg);
		return success(L);
		break;
	case StrobeLightsOn:
		return ChangeOption(L, "Strobe Lights", 1);
		break;
	case StrobeLightsOff:
		return ChangeOption(L, "Strobe Lights", 0);
		break;
	case FrontLightsOn:
		return ChangeOption(L, "Front Lights", 1);
		break;
	case FrontLightsOff:
		return ChangeOption(L, "Front Lights", 0);
		break;
	case RearLightsOn:
		return ChangeOption(L, "Rear Lights", 1);
		break;
	case RearLightsOff:
		return ChangeOption(L, "Rear Lights", 0);
		break;
	case NavLightsOn:
		return ChangeOption(L, "Nav Lights", 1);
		break;
	case NavLightsOff:
		return ChangeOption(L, "Nav Lights", 0);
		break;
	case isCharging:
		if (conditionActive(CHARGING))
			return success(L);
		else
			return fail(L);
		break;
	case isChargeComplete:
		if (conditionActive(CHARGE_COMPLETE))
			return success(L);
		else
			return fail(L);
		break;
	case isAppOnline:
		if (APPonline)
			return success(L);
		else
			return fail(L);
		break;
	case isRaining:
		if (conditionActive(RAINING))
			return success(L);
		else
			return fail(L);
		break;
	default:
		ERRORPRINT("System Action: %i\n", actionCode);
		SetCondition(BT_SCRIPT_ERROR);
		return fail(L);
		break;
	}
}

int SaveSettingsAndOptions(lua_State *L, bool useDefault)
{
	bool actionFail = false;

	FILE *fp = fopen(SAVED_SETTINGS_FILE, "w");
	if (fp)
	{
		//Settings
#define settingmacro(name, var, min, max, def) if (fprintf(fp, "'%s';%f\n", name, (useDefault ? def : var)) < 0) actionFail = true;
#include "settings.h"
#undef settingmacro

		fclose(fp);

		fp = fopen(SAVED_OPTIONS_FILE, "w");
		if (fp)
		{
			//options
#define optionmacro(name, var, min, max, def) if (fprintf(fp, "'%s';%i\n", name, (useDefault ? def : var)) < 0) actionFail = true;
#include "options.h"
#undef optionmacro

			fclose(fp);

			if (actionFail) {
				return fail(L);
			}
			else
			{
				LogRoutine("Settings and Options Saved\n");
				return success(L);
			}
		}
		else
		{
			LogError("Options.txt - %s\n", strerror(errno));
			return fail(L);
		}
	}
	else
	{
		LogError("Options.txt - %s\n", SAVED_SETTINGS_FILE, strerror(errno));
		return  fail(L);
	}
}

int LoadSettingsAndOptions(lua_State *L)
{
	char name[80];
	float setting;
	int option;
	int result;

	FILE *fp = fopen(SAVED_SETTINGS_FILE, "r");
	if (fp)
	{
		do {
			result = fscanf(fp, "'%79s';%f\n", &name, &setting);
			if (result  == 2)
			{
				//Settings
#define settingmacro(n, var, min, max, def) if (strncmp(name, n, 80) == 0) var = setting;
#include "settings.h"
#undef settingmacro
			}
			else if (result < 0)
			{
				LogError("Settings read: %s\n", strerror(errno));
				return fail(L);
			}
		} while (result > 0);

		fclose(fp);

		fp = fopen(SAVED_OPTIONS_FILE, "r");
		if (fp)
		{
			do {
				result = fscanf(fp, "'%79s';%i\n", &name, &option);
				if (result  == 2)
				{
					//options
#define optionmacro(n, var, min, max, def) if (strncmp(name, n, 80) == 0) var = option;
#include "options.h"
#undef optionmacro
				}
				else if (result < 0)
				{
					LogError("Options read: %s\n", strerror(errno));
					return fail(L);
				}
			} while (result > 0);

			fclose(fp);

			LogRoutine("Settings and Options Saved\n");
			return success(L);
		}
		else
		{
			LogError("Settings.txt: %s\n", strerror(errno));
			return  fail(L);
		}
	}
	else
	{
		LogError("Settigns.txt: %s\n", strerror(errno));
		return  fail(L);
	}
}
