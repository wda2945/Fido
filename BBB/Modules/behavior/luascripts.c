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

int LoadFromFolder(lua_State *L, char *scriptFolder);	//load chunks
int LoadFromFile(lua_State *L, char *name, char *path, int nargs);
int LoadChunk(lua_State *L, char *name, char *path);

//----------------------------------------LOADING LUA SCRIPTS
int LoadAllScripts(lua_State *L)
{
	int reply;

	reply = LoadFromFile(L, "btclass",  BEHAVIOR_TREE_CLASS, 1);
	if (reply == 0)
	{
		 lua_setglobal(L, "BT");
	}
	else return -1;

	reply += LoadFromFolder(L, INIT_SCRIPT_PATH);	//load init/default scripts first
	reply += LoadFromFolder(L, BT_LEAF_PATH);
	reply += LoadFromFolder(L, BT_ACTIVITY_PATH);
	reply += LoadFromFolder(L, HOOK_SCRIPT_PATH);
	reply += LoadFromFolder(L, GENERAL_SCRIPT_PATH);

	if (reply < 0) return -1;
	else return 0;
}

//private
const char * ChunkReader(lua_State *L, void *data, size_t *size);

//load all lua scripts in a folder
int LoadFromFolder(lua_State *L, char *scriptFolder)	//load all scripts in folder
{
	DIR *dp;
	struct dirent *ep;

	DEBUGPRINT("Script Folder: %s\n", scriptFolder);

	dp = opendir (scriptFolder);
	if (dp != NULL) {
		while ((ep = readdir (dp))) {

			DEBUGPRINT("File: %s\n", ep->d_name);

			//check whether the file is a lua script
			int len = strlen(ep->d_name);
			if (len < 5) continue;						//name too short

			char *suffix = ep->d_name + len - 4;
			if (strcmp(suffix, ".lua") != 0)
			{
				continue;	//wrong extension
			}

			char path[99];
			sprintf(path, "%s/%s", scriptFolder, ep->d_name);

			char *name = ep->d_name;
			name[len - 4] = '\0'; 						//chop off .lua suffix for name

			if (LoadFromFile(L, name, path, 0) < 0)
			{
				(void) closedir (dp);
				return -1;
			}
		}
		(void) closedir (dp);
	}
	DEBUGPRINT("Script Folder: %s done\n", scriptFolder);

	return 0;
}

//load script from a file
int LoadFromFile(lua_State *L, char *name, char *path, int nargs)
{
	int loadReply = LoadChunk(L, name, path);

	if (loadReply == LUA_OK)
	{
		//now call the chunk to initialize itself
		int status = lua_pcall(L, 0, nargs, 0);

		if (status != LUA_OK)
		{
			const char *errormsg = lua_tostring(L, -1);
			ERRORPRINT("Error: %s\n",errormsg);
			return -1;
		}
		else
		{
			DEBUGPRINT("loaded %s\n", name);
			return 0;
		}
	}
	else
	{
		ERRORPRINT("failed to load %s - %i\n", name, loadReply);
		return -1;
	}
}

//load a lua chunk from a file
FILE *chunkFile;
int LoadChunk(lua_State *L, char *name, char *path)
{
	int loadReply;
	DEBUGPRINT("Loading %s: %s\n",name, path);

	chunkFile = fopen(path, "r");

	if (!chunkFile)
	{
		ERRORPRINT("Failed to open %s\n",path);
		return -1;
	}

	loadReply = lua_load(L, ChunkReader, (void*) 0, name, "bt");

	fclose(chunkFile);

	if  (loadReply == LUA_ERRSYNTAX)
	{
		const char *errormsg = lua_tostring(L, -1);
		ERRORPRINT("lua: %s syntax error: %s\n", name, errormsg);
		return -1;
	}
	else if (loadReply != LUA_OK)
	{
		ERRORPRINT("lua_load of %s fail: %i\n", path, loadReply);
		return -1;
	}
	return 0;
}
//file reader call out
char buffer[100];
const char * ChunkReader(lua_State *L, void *data, size_t *size)
{
	int c;
	char *next = buffer;
	*size = 0;
	do {
		c = getc(chunkFile);
		if (c != EOF)
		{
			*next++ = c;
			(*size)++;
		}
		else
		{
			if (*size == 0) return NULL;
		}
	} while  (c != EOF && *size < 100);
	return buffer;
}
