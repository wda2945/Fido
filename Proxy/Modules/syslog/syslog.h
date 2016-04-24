/* 
 * File:   SysLog.h
 * Author: martin
 *
 * Created on August 7, 2013, 8:19 PM
 */

#ifndef SYSLOG_H
#define	SYSLOG_H

#include <stdio.h>
#include "pubsubdata.h"
#include "pubsub/pubsub.h"

enum {
	LOG_ALL, LOG_INFO_PLUS, LOG_WARN_PLUS, LOG_NONE
};

void _LogMessage(SysLogSeverity_enum _severity, const char *_message, const char *_file);

#define SysLog( s, ...) {char tmp[BBB_MAX_LOG_TEXT];\
    snprintf(tmp,BBB_MAX_LOG_TEXT,__VA_ARGS__);\
    tmp[BBB_MAX_LOG_TEXT-1] = 0;\
    _LogMessage(s, tmp, __BASE_FILE__);}


#if (SYSLOG_LEVEL == LOG_ALL)
#define LogRoutine( ...) {SysLog( SYSLOG_ROUTINE, __VA_ARGS__);}
#else
    #define LogRoutine( m, ...)
#endif

    #if (SYSLOG_LEVEL <= LOG_INFO_PLUS)
#define LogInfo( ...) {SysLog( SYSLOG_INFO, __VA_ARGS__);}
#else
    #define LogInfo( m, ...)
#endif

#if (SYSLOG_LEVEL <= LOG_WARN_PLUS)
#define LogWarning( ...) {SysLog( SYSLOG_WARNING, __VA_ARGS__);}
#else
    #define LogWarning( m, ...)
#endif

#define LogError(...) {SysLog(SYSLOG_ERROR, __VA_ARGS__);}

//syslog
extern FILE *logFile;

int SysLogInit();

void LogProcessMessage(psMessage_t *msg);				//process for logging

void PrintLogMessage(psMessage_t *msg);

#define tprintf(...) {char tmp[100];\
    snprintf(tmp,100,__VA_ARGS__);\
    tmp[100-1] = 0;\
    DebugPrint(tmp);}

void DebugPrint(char *logtext);

#endif	/* SYSLOG_H */

