/*
 * behavior.h
 *
 *  Created on: Jul 11, 2014
 *      Author: martin
 */

#ifndef BEHAVIOR_H_
#define BEHAVIOR_H_

#include "SoftwareProfile.h"
#include "PubSubData.h"
#include "lua.h"
#include "behavior/behavior_enums.h"

//behavior init
int BehaviorInit();

//update globals and hooks
void BehaviorProcessMessage(psMessage_t *msg);

int ReportAvailableScripts();

//scripting system
int InitScriptingSystem();
int ScriptProcessMessage(psMessage_t *msg);
int InvokeUpdate();
int AvailableScripts();

//LUA globals and scripts
int InitPilotingCallbacks(lua_State *L);
int InitProximityCallbacks(lua_State *L);
int InitSystemCallbacks(lua_State *L);
int InitLuaGlobals(lua_State *L);
int LoadAllScripts(lua_State *L);

int UpdateGlobalsFromMessage(lua_State *L, psMessage_t *msg);
void SetGlobal(lua_State *L, const char *name, float value);

extern bool MCPonline;
extern bool MCPconfigRequested;
extern bool MCPconfigured;

extern time_t lastMCPresponseTime;
extern time_t MCPconfigRequestedTime;

extern bool MOTonline;
extern bool MOTconfigRequested;
extern bool MOTconfigured;

extern time_t lastMOTresponseTime;
extern time_t MOTconfigRequestedTime;

extern bool APPonline;
extern time_t lastAPPresponseTime;

extern char behaviorName[PS_NAME_LENGTH];

//private

//BT call-back result codes
int success(lua_State *L);
int running(lua_State *L);
int fail(lua_State *L);

//For BT callbacks
int ChangeOption(lua_State *L, const char *name, int value);
int ChangeSetting(lua_State *L, const char *name, float value);

int actionReply(lua_State *L, ActionResult_enum result);

#endif /* BEHAVIOR_H_ */
