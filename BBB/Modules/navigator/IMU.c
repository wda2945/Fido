/*
 ============================================================================
 Name        : IMU.c
 Author      : Martin
 Version     :
 Copyright   : (c) 2015 Martin Lane-Smith
 Description : Reads IMU
 ============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "i2c.h"
#include "pubsubdata.h"
#include "pubsub/pubsub.h"
#include "pubsub/notifications.h"
#include "common.h"

#include "syslog/syslog.h"
#include "softwareprofile.h"

#include "IMU.h"
#include "LSM303.h"

FILE *imuDebugFile;

#ifdef IMU_DEBUG
#define DEBUGPRINT(...) tprintf( __VA_ARGS__);tfprintf(imuDebugFile, __VA_ARGS__);
#else
#define DEBUGPRINT(...) tfprintf(imuDebugFile, __VA_ARGS__);
#endif

#define ERRORPRINT(...) tprintf( __VA_ARGS__);tfprintf(imuDebugFile, __VA_ARGS__);


//thread to poll IMU
void *IMUReaderThread(void *arg);

int IMUInit()
{

	imuDebugFile = fopen("/root/logfiles/imu.log", "w");

	//prep I2C
	if (load_device_tree(IMU_I2C_OVERLAY) < 0)
	{
		ERRORPRINT("IMU I2C overlay: %s failed\n", IMU_I2C_OVERLAY);
//		return -1;
	}
	else {
		DEBUGPRINT("IMU I2C overlay OK\n", PS_UART_DEVICE);
	}

	if (i2c_setup(IMU_SCL_PIN, IMU_SDA_PIN) < 0)
	{
		ERRORPRINT("IMU I2C pinmux setup fail\n");
//		return -1;
	}

	//create IMU thread
	pthread_t thread;
	int s = pthread_create(&thread, NULL, IMUReaderThread, NULL);
	if (s != 0)
	{
		ERRORPRINT("IMU Thread: %i\n", s);
		return -1;
	}
	return 0;
}

int IMU_FD;


//thread to poll IMU
void *IMUReaderThread(void *arg)
{
	psMessage_t msg;
	int result;

	//open I2C driver
	while(1)
	{
		if ((IMU_FD = open(IMU_I2C_PATH, O_RDWR)) < 0){
			ERRORPRINT("open(%s): %s\n", IMU_I2C_PATH, strerror(errno));
			usleep(250000);
			continue;
		}
		else if (ioctl(IMU_FD, I2C_SLAVE, I2C_SLAVE_LSM) < 0){
			ERRORPRINT("I2C @ %2x: %s\n", I2C_SLAVE_LSM, strerror(errno));
			close(IMU_FD);
			usleep(250000);
			continue;
		}
		else if (LSM303_enableDefault() < 0)
		{
			ERRORPRINT("LSM303_enableDefault error\n");
			close(IMU_FD);
			usleep(250000);
			continue;
		}
		break;
	}

	DEBUGPRINT("IMU opened %s\n", IMU_I2C_PATH);

	sleep(1);

	while (1)
	{
		if (LSM303_read() >= 0)
		{
			//send raw message
			psInitPublish(msg, IMU_REPORT);

			msg.threeFloatPayload.heading = fmodf(LSM303_heading() + compassOffset, 360.0);
			msg.threeFloatPayload.pitch = LSM303_pitch();
			msg.threeFloatPayload.roll = LSM303_roll();

			RouteMessage(&msg);

			DEBUGPRINT("Compass: %f\n", msg.threeFloatPayload.heading);
			CancelCondition(IMU_ERROR);
		}
		else
		{
			ERRORPRINT("IMU Read fail\n");
			SetCondition(IMU_ERROR);
		}
		usleep(imuLoopDelay * 1000);
	}
}
