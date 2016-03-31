/*
 ============================================================================
 Name        : PrintLogLinux.c
 Author      : Martin
 Version     :
 Copyright   : (c) 2013 Martin Lane-Smith
 Description : Print log messages to stdout/file
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "PubSubData.h"
#include "syslog/syslog.h"

//print to an open stream - all processes
void PrintLogMessage(psMessage_t *msg)
{
	uint8_t severityCode;
	char *logText;
	char *severity;
	char *source;
	char printBuff[100];

	const time_t now = time(NULL);
	struct tm *timestruct = localtime(&now);

	switch(msg->header.messageType)
	{
	case SYSLOG_MSG:
		severityCode = msg->logPayload.severity;
		logText = msg->logPayload.text;
		msg->logPayload.text[PS_MAX_LOG_TEXT-1] = '\0';
		switch (msg->header.source) {
		case SUBSYSTEM_UNKNOWN:
		default:
			source = "???";
			break;
		case MCP_SUBSYSTEM:
			source = "MCP";
			break;
		case MOTOR_SUBSYSTEM:
			source = "MOT";
			break;
		case OVERMIND:
			source = "OVM";
			break;
		case APP_OVM:
		case APP_XBEE:
		case ROBO_APP:
			source = "APP";
		case XBEE:
			source = "BEE";
			break;
		}
		break;
    default:
	case BBBLOG_MSG:
		severityCode = msg->bbbLogPayload.severity;
		logText = msg->bbbLogPayload.text;
		msg->bbbLogPayload.text[BBB_MAX_LOG_TEXT-1] = '\0';
		source = msg->bbbLogPayload.file;
		break;
	}

	switch (severityCode) {
	case SYSLOG_ROUTINE:
	default:
		severity = "R";
		break;
	case SYSLOG_INFO:
		severity = "I";
		break;
	case SYSLOG_WARNING:
		severity = "W";
		break;
	case SYSLOG_ERROR:
		severity = "E";
		break;
	case SYSLOG_FAILURE:
		severity = "F";
		break;
	}
	//remove newline
	int len = (int) strlen(logText);
	if (logText[len-1] == '\n') logText[len-1] = '\0';

	snprintf(printBuff, 100, "%02i:%02i:%02i %s@%s: %s\n",
			timestruct->tm_hour, timestruct->tm_min, timestruct->tm_sec,
			severity, source, logText);

	fprintf(logFile, "%s", printBuff);
	fflush(logFile);

	printf("%s", printBuff);
}
