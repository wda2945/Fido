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
#include "PubSubData.h"
#include "pubsub/pubsub.h"

#include "syslog/syslog.h"
#include "SoftwareProfile.h"

#include "IMU.h"
#include "LSM303.h"



FILE *imuDebugFile;

#ifdef IMU_DEBUG
#define DEBUGPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(imuDebugFile, __VA_ARGS__);fflush(imuDebugFile);
#else
#define DEBUGPRINT(...) fprintf(imuDebugFile, __VA_ARGS__);fflush(imuDebugFile);
#endif

#define ERRORPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(imuDebugFile, __VA_ARGS__);fflush(imuDebugFile);


//thread to poll IMU
void *IMUReaderThread(void *arg);

int IMUInit()
{

	imuDebugFile = fopen("/root/logfiles/imu.log", "w");

	//prep I2C

	if (i2c_setup(IMU_SCL_PIN, IMU_SDA_PIN) < 0)
	{
		ERRORPRINT("IMU I2C setup fail\n");
		return -1;
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
	struct timespec request, remain;
	request.tv_sec = 0;
	psMessage_t msg;

	int result;
	struct timespec waitTime = {0,0};
	char IMU_I2CdevName[50];

	//LIDAR I2C device path
	snprintf(IMU_I2CdevName, sizeof(IMU_I2CdevName), "/dev/i2c-%d", IMU_I2C);

	//open I2C driver
	if ((IMU_FD = open(IMU_I2CdevName, O_RDWR)) < 0){
		LogError("open(%s): %s\n", IMU_I2CdevName, strerror(errno));
		return 0;
	}
	if (ioctl(IMU_FD, I2C_SLAVE, I2C_SLAVE_LSM) < 0){
		LogError("I2C @ %2x: %s\n", I2C_SLAVE_LSM, strerror(errno));
		return 0;
	}

	LSM303_enableDefault();
	
	sleep(1);

	while (1)
	{
		LSM303_read();

		//send raw message
		psInitPublish(msg, IMU_REPORT);

		msg.threeFloatPayload.heading = LSM303_heading() + COMPASS_OFFSET;
		msg.threeFloatPayload.pitch = LSM303_pitch();
		msg.threeFloatPayload.roll = LSM303_roll();

		RouteMessage(&msg);

		DEBUGPRINT("Compass: %f\n", msg.threeFloatPayload.heading);


		request.tv_nsec = imuLoopDelay * 1000000;
		int reply = nanosleep(&request, &remain);
	}
}
