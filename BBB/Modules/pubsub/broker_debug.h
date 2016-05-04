//broker_debug.h


#ifndef BROKER_DEBUG_H
#define BROKER_DEBUG_H


#ifdef BROKER_DEBUG
#define DEBUGPRINT(...) tprintf( __VA_ARGS__);tfprintf(psDebugFile, __VA_ARGS__);
#else
#define DEBUGPRINT(...) tfprintf(psDebugFile, __VA_ARGS__);
#endif

#define ERRORPRINT(...) tprintf( __VA_ARGS__);fprintf(psDebugFile, __VA_ARGS__);

#endif
