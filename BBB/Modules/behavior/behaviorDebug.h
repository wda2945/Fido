/*
 * behaviorDebug.h
 *
 *      Author: martin
 */

#ifndef BEHAVIORDEBUG_H_
#define BEHAVIORDEBUG_H_

#include "SoftwareProfile.h"

extern FILE *behDebugFile;

#ifdef BEHAVIOR_DEBUG
#define DEBUGPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(behDebugFile, __VA_ARGS__);fflush(behDebugFile);
#else
#define DEBUGPRINT(...)
#endif

#define ERRORPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(behDebugFile, __VA_ARGS__);fflush(behDebugFile);

#define ASSERT_LUA_TABLE(L, x, s) if (!lua_istable(L, x)) {ERRORPRINT("Not a LUA Table : %s\n", s); abort();}

#define ASSERT_LUA_STRING(L, x, s) if (!lua_isstring(L, x)) {ERRORPRINT("Not a LUA String : %s\n", s); abort();}

#define ASSERT_LUA_NUMBER(L, x, s) if (!lua_isnumber(L, x)) {ERRORPRINT("Not a LUA Number : %s\n", s); abort();}





#endif
