//
//  overmind.c
//
//  Created by Martin Lane-Smith on 7/2/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//
// Starts all robot processes

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "softwareprofile.h"
#include "pubsubdata.h"
#include "pubsub/pubsub.h"
#include "pubsub/notifications.h"

#include "behavior/behavior.h"
#include "syslog/syslog.h"
#include "navigator/navigator.h"
#include "navigator/GPS.h"
#include "navigator/IMU.h"
#include "autopilot/autopilot.h"
#include "scanner/scanner.h"
#include "agent/agent.h"
#include "common.h"

int *pidof (char *pname);

FILE *mainDebugFile;

#ifdef MAIN_DEBUG
#define DEBUGPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(mainDebugFile, __VA_ARGS__);fflush(mainDebugFile);
#else
#define DEBUGPRINT(...) fprintf(mainDebugFile, __VA_ARGS__);fflush(mainDebugFile);
#endif

#define ERRORPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(mainDebugFile, __VA_ARGS__);fflush(mainDebugFile);

#define PROCESS_NAME "fido.elf"

void SIGHUPhandler(int sig);
int SIGHUPflag = 0;
void fatal_error_signal (int sig);

int main(int argc, const char * argv[])
{
	char *initFail = "";
	int reply;

	mainDebugFile = fopen("/root/logfiles/fidomain.log", "w");

	LogInfo("Overmind starting");

	//kill any others
	int *pids = pidof(PROCESS_NAME);
	//kill
	while (*pids != -1) {
		if (*pids != getpid())
		{
			kill(*pids, SIGTERM);
			LogInfo("Killed pid %i (%s)", *pids, PROCESS_NAME);
		}
		pids++;
	}

	DEBUGPRINT("main() loading dto1...\n");

	//load universal dtos
	if (!load_device_tree("cape-universal")) {
		//error enabling pins
		ERRORPRINT( "*** Load 'cape-universal' fail\n");
//		initFail = "dta1";
	}
	else DEBUGPRINT("...loaded OK\n");

	DEBUGPRINT("main() loading dto2...\n");
	if (!load_device_tree("cape-univ-hdmi")) {
		//error enabling pins
		ERRORPRINT( "*** Load 'cape-univ-hdmi' fail\n");
//		initFail = "dta2";
	}
	else DEBUGPRINT("...loaded OK\n");

	//init other threads, etc.
	DEBUGPRINT("main() start init\n");

	//initialize the broker queues
	BrokerQueueInit(100);

	//syslog
	reply = SysLogInit(argc, argv);
	if (reply != 0)
	{
		ERRORPRINT("SysLogInit() Fail\n");
		initFail = "log";
	}
	else DEBUGPRINT("SysLogInit() OK\n");

	//start navigator
	reply  =NavigatorInit();
	if (reply != 0)
	{
		ERRORPRINT("NavigatorInit() OK\n");
		initFail = "nav";
	}
	else DEBUGPRINT("NavigatorInit() OK\n");

	//start autopilot
	reply = AutopilotInit();
	if (reply != 0)
	{
		ERRORPRINT("AutopilotInit() fail\n");
		initFail = "pilot";
	}
	else DEBUGPRINT("AutopilotInit() OK\n");

	//start IMU
	reply = IMUInit();
	if (reply != 0)
	{
		ERRORPRINT("IMUInit() fail\n");
		initFail = "imu";
	}
	else DEBUGPRINT("IMUInit() OK\n");

	//start GPS
	reply = GPSInit();
	if (reply != 0)
	{
		ERRORPRINT("GPSInit() fail\n");
		initFail = "gps";
	}
	else DEBUGPRINT("GPSInit() OK\n");

	//start scanner
// 	reply = ScannerInit();
// 	if (reply != 0)
// 	{
// 		ERRORPRINT("ScannerInit() fail\n");
// 		initFail = "scanner";
// 	}
// 	else {
// 		DEBUGPRINT("ScannerInit() OK\n");
// 	}

	//start behaviors
	reply = BehaviorInit();
	if (reply != 0)
	{
		ERRORPRINT("BehaviorInit() fail\n");
		initFail = "behavior";
	}
	else DEBUGPRINT("BehaviorInit() OK\n");

	//PubSub broker
	reply = PubSubInit();
	if (reply != 0)
	{
		ERRORPRINT("PubSubInit() fail\n");
		initFail = "pubsub";
	}
	else DEBUGPRINT("PubSubInit() OK\n");

	//Serial broker
	reply=SerialBrokerInit();
	if (reply != 0)
	{
		ERRORPRINT("SerialBrokerInit() fail\n");
		initFail = "serial";
	}
	else DEBUGPRINT("SerialBrokerInit() OK\n");

	//IP broker
	reply = AgentInit();
	if (reply != 0)
	{
		ERRORPRINT("AgentInit() fail\n");
		initFail = "agent";
	}
	else DEBUGPRINT("AgentInit() OK\n");

	//Responder
	reply=ResponderInit();
	if (reply != 0)
	{
		ERRORPRINT("ResponderInit() fail\n");
		initFail = "responder";
	}
	else DEBUGPRINT("ResponderInit() OK\n");

	if (strlen(initFail) > 0)
	{
		psMessage_t msg;
		psInitPublish(msg, ALERT);
		snprintf(msg.namePayload.name, PS_NAME_LENGTH, "Fail: %s", initFail);
		RouteMessage(&msg);

		LogError("Startup: %s", initFail);
		SetCondition(OVM_INIT_ERROR);
		sleep(10);
		return -1;
	}

	if (strlen(initFail) == 0) LogInfo("Init complete");

	sleep(10);

	{
		psMessage_t msg2;
		psInitPublish(msg2, SS_ONLINE);
		strcpy(msg2.responsePayload.subsystem, "OVM");
		msg2.responsePayload.flags = RESPONSE_FIRST_TIME | ((strlen(initFail) > 0) ? RESPONSE_INIT_ERRORS : 0);
		RouteMessage(&msg2);
	}

	if (getppid() == 1)
	{
		//child of init/systemd
		if (signal(SIGHUP, SIGHUPhandler) == SIG_ERR)
		{
			LogError("SIGHUP err: %s", strerror(errno));
		}
		else
		{
			DEBUGPRINT("SIGHUP handler set\n");
		}
		//close stdio
		fclose(stdout);
		fclose(stderr);
		stdout = fopen("/dev/null", "w");
		stderr = fopen("/dev/null", "w");
	}
	else
	{
		signal(SIGILL, fatal_error_signal);
		signal(SIGABRT, fatal_error_signal);
		signal(SIGIOT, fatal_error_signal);
		signal(SIGBUS, fatal_error_signal);
		signal(SIGFPE, fatal_error_signal);
		signal(SIGSEGV, fatal_error_signal);
		signal(SIGTERM, fatal_error_signal);
		signal(SIGCHLD, fatal_error_signal);
		signal(SIGSYS, fatal_error_signal);
		signal(SIGCHLD, fatal_error_signal);
	}

	while(1)
	{
		sleep(1);
		if (SIGHUPflag)
		{
			//SIGHUP Signal used to reload lua scripts
			psMessage_t msg;
			psInitPublish(msg, RELOAD);
			RouteMessage(&msg);

			SIGHUPflag = 0;
		}
	}

	return 0;
}
//SIGHUP
void SIGHUPhandler(int sig)
{
	SIGHUPflag = 1;
	LogError("SIGHUP signal");
}

//other signals
volatile sig_atomic_t fatal_error_in_progress = 0;
void fatal_error_signal (int sig)
{
	/* Since this handler is established for more than one kind of signal, it might still get invoked recursively by delivery of some other kind of signal. Use a static variable to keep track of that. */
	if (fatal_error_in_progress)
		raise (sig);
	fatal_error_in_progress = 1;

	LogError("Signal %i raised", sig);
	sleep(1);	//let there be printing

	/* Now re-raise the signal. We reactivate the signal�s default handling, which is to terminate the process. We could just call exit or abort,
but re-raising the signal sets the return status
from the process correctly. */
	signal (sig, SIG_DFL);
	raise (sig);
}
