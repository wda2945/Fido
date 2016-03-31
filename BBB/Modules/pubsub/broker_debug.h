//broker_debug.h


#ifndef BROKER_DEBUG_H
#define BROKER_DEBUG_H


#ifdef BROKER_DEBUG
#define DEBUGPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(psDebugFile, __VA_ARGS__);fflush(psDebugFile);
#else
#define DEBUGPRINT(...) fprintf(psDebugFile, __VA_ARGS__);fflush(psDebugFile);
#endif

#define ERRORPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(psDebugFile, __VA_ARGS__);fflush(psDebugFile);

#endif
