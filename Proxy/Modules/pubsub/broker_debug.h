//broker_debug.h


#ifndef BROKER_DEBUG_H
#define BROKER_DEBUG_H

#include "softwareprofile.h"
#include "syslog/syslog.h"

extern FILE *psDebugFile;

#ifdef BROKER_DEBUG
#define DEBUGPRINT(...) {tprintf( __VA_ARGS__);fprintf(psDebugFile, __VA_ARGS__);}
#else
#define DEBUGPRINT(...) fprintf(psDebugFile, __VA_ARGS__);
#endif

#define ERRORPRINT(...) {tprintf( __VA_ARGS__);fprintf(psDebugFile, __VA_ARGS__);}

#endif
