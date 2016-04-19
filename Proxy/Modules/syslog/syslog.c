/*
 * SysLog.c
 *
 *  Created on: Sep 10, 2014
 *      Author: martin
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "softwareprofile.h"
#include "pubsubdata.h"
#include "pubsub/pubsub.h"

#include "syslog/syslog.h"
//#include "behavior/behavior.h"

//Sys Log queue
BrokerQueue_t logQueue = BROKER_Q_INITIALIZER;

void *LoggingThread(void *arg);

FILE *logFile = NULL;

struct tm *logopentime;

int rotateLogFile()
{
	if (logFile)
	{
		fclose(logFile);
	}

	char logfilepath[200];
	const time_t now = time(NULL);
	logopentime = localtime(&now);

	snprintf(logfilepath, 200, "%sSYS_%4i_%02i_%02i_%02i_%02i_%02i.log", LOGFILE_FOLDER,
			logopentime->tm_year + 1900, logopentime->tm_mon + 1, logopentime->tm_mday,
			logopentime->tm_hour, logopentime->tm_min, logopentime->tm_sec);

	logFile = fopen(logfilepath, "w");
	if (logFile == NULL)
	{
		logFile = stderr;
		fprintf(stderr, "syslog: fopen(%s) fail (%s)\n", logfilepath, strerror(errno));
		return -1;
	}
	else {
		fprintf(stderr, "syslog: Logfile opened on %s\n", logfilepath);
		return 0;
	}
}

int SysLogInit()
{
	if (rotateLogFile() < 0)
		return -1;

	//start log print threads
	pthread_t thread;

	int s = pthread_create(&thread, NULL, LoggingThread, NULL);
	if (s != 0)
	{
		fprintf(stderr, "syslog: pthread_create fail (%s)\n", strerror(errno));
		return -1;
	}
	return 0;
}
//writes log messages to the logfile
void *LoggingThread(void *arg)
{
	NotificationMask_t currentActiveConditions[MASK_PAYLOAD_COUNT];
	NotificationMask_t currentValidConditions[MASK_PAYLOAD_COUNT];

	int i;
	for (i=0; i< MASK_PAYLOAD_COUNT; i++)
	{
		currentActiveConditions[i] = currentValidConditions[i] = 0;
	}

	{
		psMessage_t msg;
		psInitPublish(msg, BBBLOG_MSG);
		msg.bbbLogPayload.severity = SYSLOG_ROUTINE;
		strcpy(msg.bbbLogPayload.file, "log");
		strcpy(msg.bbbLogPayload.text, "Logfile started");

		PrintLogMessage(&msg);
	}

	while (1)
	{
		psMessage_t *msg = GetNextMessage(&logQueue);

		const time_t now = time(NULL);
		struct tm *timestruct = localtime(&now);

		if (logopentime->tm_mday != timestruct->tm_mday) rotateLogFile();

		switch (msg->header.messageType)
		{
		case BBBLOG_MSG:
			//check whether we need to forward a copy to PIC/APP
			switch (msg->bbbLogPayload.severity) {
			case SYSLOG_ERROR:
			case SYSLOG_FAILURE:
			{
				//make a SYSLOG_MSG
				psMessage_t newMsg;
				psInitPublish(newMsg, SYSLOG_MSG);
				newMsg.header.source = XBEE;
				newMsg.logPayload.severity = msg->bbbLogPayload.severity;
				snprintf(newMsg.logPayload.text, PS_MAX_LOG_TEXT-1, "%s: %s",
						msg->bbbLogPayload.file, msg->bbbLogPayload.text);
				RouteMessage(&newMsg);
			}
			break;
			default:
				break;
			}
			PrintLogMessage(msg);
			DoneWithMessage(msg);
			break;
			case SYSLOG_MSG:
				PrintLogMessage(msg);
				DoneWithMessage(msg);
				break;
			case NOTIFY:
			{
				psMessage_t logMsg;
				psInitPublish(logMsg, BBBLOG_MSG);
				strncpy(logMsg.bbbLogPayload.file, subsystemNames[msg->header.source], 4);
				logMsg.bbbLogPayload.severity = SYSLOG_ROUTINE;
				snprintf(logMsg.bbbLogPayload.text, BBB_MAX_LOG_TEXT, "Event: %s",
						eventNames[msg->intPayload.value]);
				PrintLogMessage(&logMsg);
			}
				DoneWithMessage(msg);
				break;

			case CONDITIONS:
			{
				int j;
				psMessage_t logMsg;

				psInitPublish(logMsg, BBBLOG_MSG);
				strncpy(logMsg.bbbLogPayload.file, subsystemNames[msg->header.source], 4);
				logMsg.bbbLogPayload.severity = SYSLOG_ROUTINE;

				for (j=0; j<MASK_PAYLOAD_COUNT; j++)
				{
					NotificationMask_t valid = msg->maskPayload.valid[j];
					NotificationMask_t value = msg->maskPayload.value[j];

					NotificationMask_t newSets = (value & valid) & ~(currentActiveConditions[j] & currentValidConditions[j]);
					NotificationMask_t newClears = (valid & ~value) & ~(currentValidConditions[j] & ~currentActiveConditions[j]);

					if ((newSets != 0) || (newClears != 0))
					{
						for (int i=0; i<CONDITION_COUNT; i++)
						{

							NotificationMask_t m = NOTIFICATION_MASK(i);

							if (m & newSets)
							{
								snprintf(logMsg.bbbLogPayload.text, BBB_MAX_LOG_TEXT, "Set Condition: %s",
										conditionNames[i + 64 * j]);
								PrintLogMessage(&logMsg);
							}
							if (m & newClears)
							{
								snprintf(logMsg.bbbLogPayload.text, BBB_MAX_LOG_TEXT, "Clear Condition: %s",
										conditionNames[i + 64 * j]);
								PrintLogMessage(&logMsg);
							}
						}
					}
					currentActiveConditions[j] |= (valid & value);
					currentActiveConditions[j] &= ~(valid & ~value);
					currentValidConditions[j] |= valid;
				}
			}
				DoneWithMessage(msg);
				break;
			default:
				DoneWithMessage(msg);
				break;
		}
	}
}
//accept a message from the broker for logging
void LogProcessMessage(psMessage_t *msg)
{
	switch(msg->header.messageType)
	{
	case SYSLOG_MSG:		//message from other parts of the system
		if (msg->header.source != XBEE)	//skip forwarded copies
			CopyMessageToQ(&logQueue, msg);
		break;
	case BBBLOG_MSG:		//BBB originated messages
		CopyMessageToQ(&logQueue, msg);
		break;
	case NOTIFY:
	case CONDITIONS:
		CopyMessageToQ(&logQueue, msg);
		break;
	}
}

//create new log message. called by macros in syslog.h
void _LogMessage(SysLogSeverity_enum _severity, const char *_message, const char *_file)
{
	if (SYSLOG_LEVEL <= _severity ) {
		const char *filecomponent = (strrchr(_file, '/') + 1);
		if (!filecomponent) filecomponent = _file;
		if (!filecomponent) filecomponent = "****";

		psMessage_t msg;
		psInitPublish(msg, BBBLOG_MSG);
		msg.bbbLogPayload.severity = _severity;
		strncpy(msg.bbbLogPayload.file, filecomponent, 4);
		msg.bbbLogPayload.file[4] = 0;
		strncpy(msg.bbbLogPayload.text, _message, BBB_MAX_LOG_TEXT);

		//publish a copy
		RouteMessage(&msg);
	}
}
