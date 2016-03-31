/* 
 * File:   SysLog.h
 * Author: martin
 *
 * Created on August 7, 2013, 8:19 PM
 */

#ifndef SYSLOG_H
#define	SYSLOG_H

#include "PubSubData.h"
#include "SoftwareProfile.h"
#include "PortablePrint.h"

#ifdef	__cplusplus
extern "C" {
#endif

    int SysLogInit();

    //generic debug logging call
    void Syslog_write_string(const unsigned char *buffer);
    //generic logging call for use during init and FAILS
    void _ExceptionMessage(const unsigned char *_message);

    //generic call to log a message
    //requires the system to be running
    void _LogMessage(SysLogSeverity_enum _severity, const char *_message);

#define SysLog( s, ...) {char tmp[PS_MAX_LOG_TEXT];\
    snprintf(tmp,PS_MAX_LOG_TEXT-1,__VA_ARGS__);\
    tmp[PS_MAX_LOG_TEXT-1] = 0;\
    _LogMessage(s, tmp);}


#if ((SYSLOG_LEVEL == LOG_ALL) || (LOG_TO_SERIAL == LOG_ALL))
#define LogRoutine( ...) SysLog( SYSLOG_ROUTINE, __VA_ARGS__)
#else
    #define LogRoutine( m, ...)
#endif

#if ((SYSLOG_LEVEL < LOG_INFO_PLUS) || (LOG_TO_SERIAL < LOG_INFO_PLUS))
#define LogInfo( ...) SysLog( SYSLOG_INFO, __VA_ARGS__)
#else
    #define LogInfo( m, ...)
#endif

#if ((SYSLOG_LEVEL < LOG_WARN_PLUS) || (LOG_TO_SERIAL < LOG_WARN_PLUS))
#define LogWarning( ...) SysLog( SYSLOG_WARNING, __VA_ARGS__)
#else
    #define LogWarning( m, ...)
#endif

#if ((SYSLOG_LEVEL < LOG_NONE) || (LOG_TO_SERIAL < LOG_NONE))
#define LogError(  ...) SysLog(SYSLOG_ERROR, __VA_ARGS__)
#else
    #define LogError( m, ...)
#endif

#ifdef DEBUG_PRINT

#define DebugPrint( ...)  {char tmp[81];\
    snprintf(tmp,80,__VA_ARGS__);\
    tmp[80] = 0;\
    Syslog_write_string(tmp);}
#else
    #define DebugPrint(...)
#endif

    void GenerateRunTimeTaskStats();
    void GenerateRunTimeSystemStats();

#ifdef	__cplusplus
}
#endif

#endif	/* SYSLOG_H */

