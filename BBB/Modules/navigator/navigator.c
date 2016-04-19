/*
 ============================================================================
 Name        : navigator.c
 Author      : Martin
 Version     :
 Copyright   : (c) 2013 Martin Lane-Smith
 Description : Receives IMU (compass) updates & odometry messages from the robot and consolidates a POSE report.
 	 	 	   Receives MOVEMENT requests from the overmind and derives MOVE commands to the robot.
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
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "i2c.h"
#include "uart.h"
#include "common.h"
#include "pubsubdata.h"
#include "pubsub/pubsub.h"
#include "pubsub/brokerq.h"
#include "pubsub/notifications.h"

#include "syslog/syslog.h"
#include "softwareprofile.h"
#include "helpers.h"
#include "navigator/navigator.h"
#include "navigator/kalman.h"


FILE *navDebugFile;

#ifdef NAVIGATOR_DEBUG
#define DEBUGPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(navDebugFile, __VA_ARGS__);fflush(navDebugFile);
#else
#define DEBUGPRINT(...) fprintf(navDebugFile, __VA_ARGS__);fflush(navDebugFile);
#endif

#define ERRORPRINT(...) LogError(__VA_ARGS__);fprintf(navDebugFile, __VA_ARGS__);fflush(navDebugFile);

enum {NO_DATA, GOT_IMU, GOT_GPS, GOOD_POSE} navigationState = NO_DATA;
char *navStates[] = {"No Data", "IMU only", "GPS + IMU", "Good Pose"};

//main navigator thread
void *NavigatorThread(void *arg);

BrokerQueue_t navigatorQueue = BROKER_Q_INITIALIZER;

int NavigatorInit()
{
	int i;
	navDebugFile = fopen("/root/logfiles/navigator.log", "w");

	//create navigator thread
	pthread_t thread;
	int s = pthread_create(&thread, NULL, NavigatorThread, NULL);
	if (s != 0)
	{
		ERRORPRINT("Nav Thread: %i\n", s);
		return -1;
	}

	return 0;
}

time_t FixWaitStart = 0;
ActionResult_enum NavigatorIsGoodPose(bool waitForFix)
{
	if (navigationState == GOOD_POSE)
	{
		FixWaitStart = 0;
		return ACTION_SUCCESS;
	}
	else if (!waitForFix) return ACTION_FAIL;
	else
	{
		if (FixWaitStart == 0)
		{
			FixWaitStart = time(NULL);
			return ACTION_RUNNING;
		}
		else
		{
			if (FixWaitStart + FixWaitTime < time(NULL))
			{
				FixWaitStart = 0;
				return ACTION_FAIL;
			}
			else return ACTION_RUNNING;
		}
	}
}

//func to receive raw nav messages
void NavigatorProcessMessage(psMessage_t *msg)
{
	CopyMessageToQ(&navigatorQueue, msg);
}

void *NavigatorThread(void *arg)
{
	psMessage_t *msg;

	//Robot pose
	double roll = 0;
	double pitch = 0;

	//incoming data
	psPositionPayload_t GPS_report;

	ps3FloatPayload_t IMU_report;
	psOdometryPayload_t ODO_report;

	time_t latestGPSFixTime = 0;
	time_t oldestGPSFixTime = 0;
	time_t latestIMUReportTime = 0;
	time_t latestOdoReportTime = 0;
	time_t latestReportTime = 0;
	time_t latestAppReportTime = 0;

	time_t getFixEndTime = 0;
	BrokerQueue_t getFixQueue = BROKER_Q_INITIALIZER;
	BrokerQueue_t varianceQueue = BROKER_Q_INITIALIZER;

	double latitudeSum, longitudeSum;
	int sampleCount = 0;

	//latest pose report
	psMessage_t poseMsg;
	psPosePayload_t lastPoseMsg;

	bool GPSGood;
	bool IMUGood;
	bool reportRequired;

	//set up filters
	////////////////////////////////////////////////////////////////////////
	//heading filter - 2 dimensions system (h, dh), 1 dimension measurement
	KalmanFilter HeadingFilter = alloc_filter(2, 1);
	set_identity_matrix(HeadingFilter.state_transition);
#define SET_HEADING_CHANGE(H) HeadingFilter.state_transition.data[0][1] = H;
	//then predict(f)

	/* We only observe (h) each time */
	set_matrix(HeadingFilter.observation_model,
		     1.0, 0.0);
#define SET_HEADING_OBSERVATION(H) set_matrix(HeadingFilter.observation, H);
	//then estimate(f)

	/* Noise in the world. */
	double pos = 10.0;
	set_matrix(HeadingFilter.process_noise_covariance,
		     pos, 0.0,
		     0.0, 1.0);
#define SET_HEADING_PROCESS_NOISE(N) HeadingFilter.process_noise_covariance.data[0][0] = N;

	/* Noise in our observation */
	set_matrix(HeadingFilter.observation_noise_covariance, 4.0);
#define SET_HEADING_OBSERVATION_NOISE(N) set_matrix(HeadingFilter.observation_noise_covariance, N);

	/* The start.heading is unknown, so give a high variance */
	set_matrix(HeadingFilter.state_estimate, 0.0, 0.0);
	set_identity_matrix(HeadingFilter.estimate_covariance);
	scale_matrix(HeadingFilter.estimate_covariance, 100000.0);
#define GET_HEADING NORMALIZE_HEADING((int) HeadingFilter.state_estimate.data[0][0])	//always 0 to 359

//	PRINT_MATRIX(HeadingFilter.state_transition);
//	PRINT_MATRIX(HeadingFilter.process_noise_covariance);
//	PRINT_MATRIX(HeadingFilter.observation_model);
//	PRINT_MATRIX(HeadingFilter.observation);
//	PRINT_MATRIX(HeadingFilter.observation_noise_covariance);
//	PRINT_MATRIX(HeadingFilter.state_estimate);
//	PRINT_MATRIX(HeadingFilter.estimate_covariance);

	////////////////////////////////////////////////////////////////////////////
	//location filter - 4 dimensions system (n,e,dn,de), 2 dimensions measurement (x,y)
	KalmanFilter LocationFilter = alloc_filter(4, 2);
	set_identity_matrix(LocationFilter.state_transition);
	//PREDICT STEP
#define SET_NORTHING_CHANGE(N) LocationFilter.state_transition.data[0][2] = N;
#define SET_EASTING_CHANGE(E) LocationFilter.state_transition.data[1][3] = E;
	//then predict(f)

	/* We observe (x, y) in each time step */
	set_matrix(LocationFilter.observation_model,
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0);
#define SET_LOCATION_OBSERVATION(N,E) set_matrix(LocationFilter.observation, N, E);
	//then estimate(f)

	/* Noise in the world. */
	set_matrix(LocationFilter.process_noise_covariance,
			pos, 0.0, 0.0, 0.0,
			0.0, pos, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 0.0, 1.0);
#define SET_LOCATION_PROCESS_NOISE(N) LocationFilter.state_transition.data[0][0] = N;LocationFilter.state_transition.data[1][1] = N;

	/* Noise in our observation */
	set_matrix(LocationFilter.observation_noise_covariance,
			1000.0, 0.0,
			0.0, 1000.0);
#define SET_LOCATION_OBSERVATION_NOISE(N, E) LocationFilter.observation_noise_covariance.data[0][0] = N;LocationFilter.observation_noise_covariance.data[1][1] = E;
#define VERY_LARGE_COVARIANCE 1000000000.0
	/* The start position is unknown, so give a high variance */
	set_matrix(LocationFilter.state_estimate, 347.8, 328.0, 0.0, 0.0);
	set_identity_matrix(LocationFilter.estimate_covariance);
	scale_matrix(LocationFilter.estimate_covariance, 100000.0);
#define GET_LATITUDE 	(LocationFilter.state_estimate.data[0][0])
#define GET_LONGITUDE 	(LocationFilter.state_estimate.data[1][0])


//	PRINT_MATRIX(LocationFilter.state_transition);
//	PRINT_MATRIX(LocationFilter.process_noise_covariance);
//	PRINT_MATRIX(LocationFilter.observation_model);
//	PRINT_MATRIX(LocationFilter.observation);
//	PRINT_MATRIX(LocationFilter.observation_noise_covariance);
//	PRINT_MATRIX(LocationFilter.state_estimate);
//	PRINT_MATRIX(LocationFilter.estimate_covariance);

	///////////////////////////////////////////////////////////////////////////////

	while (1) {

		msg = GetNextMessage(&navigatorQueue);

		DEBUGPRINT("Navigator RX: %s\n", psLongMsgNames[msg->header.messageType]);

		reportRequired = false;

		switch (msg->header.messageType)
		{
		case GPS_REPORT:
		{
			if ((msg->positionPayload.gpsStatus && msg->positionPayload.HDOP <= 10) || simGPS)
			{
				if (oldestGPSFixTime = 0) oldestGPSFixTime = time(NULL);
				//save the fix
				latestGPSFixTime = time(NULL);
				GPS_report = msg->positionPayload;

				//update the filter
				SET_NORTHING_CHANGE(0.0);
				SET_EASTING_CHANGE(0.0);
				SET_LOCATION_OBSERVATION(GPS_report.latitude, GPS_report.longitude);
				SET_LOCATION_OBSERVATION_NOISE(GPS_report.HDOP * 100, GPS_report.HDOP * 100);

				predict(LocationFilter);
				estimate(LocationFilter);

				DEBUGPRINT("GPS: %fN, %fE\n", GET_LATITUDE, GET_LONGITUDE);

				if (getFixEndTime > time(NULL))
				{
					//fix averaging
					//save message
					AppendQueueEntry(&getFixQueue, (BrokerQueueEntry_t *) msg);
				}
				else if (!isQueueEmpty(&getFixQueue))
				{
					AppendQueueEntry(&getFixQueue, (BrokerQueueEntry_t *) msg);
					latitudeSum = longitudeSum = 0;
					sampleCount = 0;
					//get average
					while (!isQueueEmpty(&getFixQueue))
					{
						psMessage_t *fixmsg = GetNextMessage(&getFixQueue);		//only if not empty - no wait

						latitudeSum += fixmsg->positionPayload.latitude;
						longitudeSum  += fixmsg->positionPayload.longitude;
						sampleCount++;

						DEBUGPRINT("#%i GetFix: %fN, %fE\n",
								sampleCount,fixmsg->positionPayload.latitude,
								fixmsg->positionPayload.longitude);

						AppendQueueEntry(&varianceQueue, (BrokerQueueEntry_t *) fixmsg);
					}
					double latitude = latitudeSum / sampleCount;		//mean
					double longitude = longitudeSum / sampleCount;
					double latitudeSumErrors 	= 0;
					double longitudeSumErrors 	= 0;
					double HDOP					= 0;
					//get variance
					while (!isQueueEmpty(&varianceQueue))
					{
						psMessage_t *fixmsg = GetNextMessage(&varianceQueue);		//only if not empty - no wait

						double errorN = latitude - fixmsg->positionPayload.latitude;
						double errorE = longitude - fixmsg->positionPayload.longitude;

						latitudeSumErrors = errorN * errorN;
						longitudeSumErrors = errorE * errorE;

						HDOP   = fixmsg->positionPayload.HDOP;

						DoneWithMessage(fixmsg);
					}
					double latitudeVariance = latitudeSumErrors / sampleCount;
					double longitudeVariance = longitudeSumErrors / sampleCount;

					//load into Kalman Filter
					SET_NORTHING_CHANGE(0.0);
					SET_EASTING_CHANGE(0.0);
					predict(LocationFilter);
					SET_LOCATION_OBSERVATION(latitude, longitude);
					SET_LOCATION_OBSERVATION_NOISE(latitudeVariance + HDOP, longitudeVariance + HDOP);
					estimate(LocationFilter);

					DEBUGPRINT("Fix:: %f, %f. Var %f, %f\n", GET_LONGITUDE, GET_LATITUDE, latitudeVariance, longitudeVariance);

					GPSGood = ((sampleCount > 10) && (HDOP <= 10));
				}
				else
				{
					DoneWithMessage(msg);
				}
			}
			else
			{
				oldestGPSFixTime = 0;
				GPSGood = false;
				DoneWithMessage(msg);
			}
		}
		break;
		case IMU_REPORT:
		{
			latestIMUReportTime = time(NULL);
			IMU_report = msg->threeFloatPayload;
			IMUGood = true;
			//update heading belief
			roll = IMU_report.roll;
			pitch = IMU_report.pitch;
			SET_HEADING_OBSERVATION(IMU_report.heading);
			SET_HEADING_OBSERVATION_NOISE(5.0)
			SET_HEADING_CHANGE(0.0);
			predict(HeadingFilter);
			estimate(HeadingFilter);

			DEBUGPRINT("IMU heading: %i\n", GET_HEADING);
		}
		DoneWithMessage(msg);
		break;
		case ODOMETRY:
		{
			latestOdoReportTime = time(NULL);
			ODO_report = msg->odometryPayload;

			//update heading belief
			double veerAngle = (ODO_report.portMovement - ODO_report.starboardMovement) / (FIDO_RADIUS * 2);
			SET_HEADING_CHANGE(veerAngle);
			SET_HEADING_OBSERVATION(HeadingFilter.predicted_state.data[0][0]);
			SET_HEADING_OBSERVATION_NOISE(VERY_LARGE_COVARIANCE)
			predict(HeadingFilter);
			estimate(HeadingFilter);

			DEBUGPRINT("ODO heading: %f\n", GET_HEADING);

			double hRadians = DEGREESTORADIANS(GET_HEADING);
			double movement = (ODO_report.portMovement + ODO_report.starboardMovement) / 2;

			//update location belief
			SET_NORTHING_CHANGE(movement * cosf(hRadians));
			SET_EASTING_CHANGE(movement * sinf(hRadians));
			SET_LOCATION_OBSERVATION(LocationFilter.predicted_state.data[0][0], LocationFilter.predicted_state.data[1][0]);
			SET_LOCATION_OBSERVATION_NOISE(VERY_LARGE_COVARIANCE, VERY_LARGE_COVARIANCE);
			predict(LocationFilter);
			estimate(LocationFilter);

			DEBUGPRINT("ODO location: %f, %f\n", GET_LATITUDE, GET_LONGITUDE);
		}
		DoneWithMessage(msg);
		break;
		default:
			DoneWithMessage(msg);
			break;
		}


		if (time(NULL) - latestGPSFixTime > RAW_DATA_TIMEOUT)
		{
			latestGPSFixTime = 0;
			GPSGood = false;
		}
		if (time(NULL) - latestIMUReportTime > RAW_DATA_TIMEOUT)
		{
			IMUGood = false;
		}

		int savedState = navigationState;

		if (GPSGood) simGPS = false;

		switch (navigationState)
		{
		case NO_DATA:
			if (IMUGood)
			{
				navigationState = GOT_IMU;
			}
			break;
		case GOT_IMU:
			if (!IMUGood)
			{
				navigationState = NO_DATA;
			}
			else
			{
				if (GPSGood || simGPS)
				{
					navigationState = GOT_GPS;
				}
			}
			break;
		case GOT_GPS:
			if (!GPSGood && !simGPS)
			{
				navigationState = GOT_IMU;
			}
			else
			{
				if ((oldestGPSFixTime + GPS_STABILITY_TIME < time(NULL)) || simGPS)
				{
					//GPS has been stable long enough
					navigationState = GOOD_POSE;
					SetCondition(GPS_FIX);
					reportRequired = true;
				}
			}
			break;
		case GOOD_POSE:
			if (!GPSGood && !simGPS)
			{
				if (latestGPSFixTime + GPS_FIX_LOST_TIMEOUT < time(NULL))
				{
					CancelCondition(GPS_FIX);
					navigationState = GOT_IMU;
					reportRequired = true;
				}
			}
			else
			{
				//whether to report
#define max(a,b) (a>b?a:b)

				if ((abs(lastPoseMsg.position.latitude - poseMsg.posePayload.position.latitude) > REPORT_LOCATION_CHANGE) ||
						(abs(lastPoseMsg.position.longitude - poseMsg.posePayload.position.longitude) > REPORT_LOCATION_CHANGE) ||
						(abs(NORMALIZE_HEADING(lastPoseMsg.orientation.heading - poseMsg.posePayload.orientation.heading)) > REPORT_BEARING_CHANGE) ||
						(latestReportTime + REPORT_MAX_INTERVAL > time(NULL)))
				{
					reportRequired = true;
				}
			}
			break;
		}

		if (savedState != navigationState)
		{
			LogInfo("NAV: State: %s\n", navStates[navigationState]);
		}

		if (reportRequired)
		{
			memset(&poseMsg, 0, sizeof(poseMsg));
			psInitPublish(poseMsg, POSE);

			if (GPSGood) {
				poseMsg.posePayload.position.latitude = GET_LATITUDE;
				poseMsg.posePayload.position.longitude = GET_LONGITUDE;
				settingLatitude = GET_LATITUDE;
				settingLongitude = -GET_LONGITUDE;
			}
			else if (simGPS)
			{
				poseMsg.posePayload.position.latitude = settingLatitude;
				poseMsg.posePayload.position.longitude = -settingLongitude;
			}

			if (IMUGood) {
				poseMsg.posePayload.orientation.roll = roll;
				poseMsg.posePayload.orientation.pitch = pitch;
				poseMsg.posePayload.orientation.heading = GET_HEADING;
			}
			poseMsg.posePayload.orientationValid = IMUGood;
			poseMsg.posePayload.positionValid = (GPSGood || simGPS);

			lastPoseMsg = poseMsg.posePayload;
			RouteMessage(&poseMsg);
			latestReportTime = time(NULL);

			if (latestAppReportTime + appReportInterval < time(NULL)){
				LogRoutine("NAV: N=%f, E=%f, H=%f\n", poseMsg.posePayload.position.latitude, poseMsg.posePayload.position.longitude, lastPoseMsg.orientation.heading )
				//send another to the App
				psInitPublish(poseMsg, POSEREP);
				RouteMessage(&poseMsg);
				latestAppReportTime = time(NULL);
			}
		}

		//TODO: GPS stats collection if stationary.
	}
}

/* Subtract the ‘struct timeval’ values X and Y, storing the result in RESULT.
Return 1 if the difference is negative, otherwise 0. */
int timeval_subtract (result, x, y)
struct timeval *result, *x, *y;
{
	/* Perform the carry for the later subtraction by updating y. */ if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
	/* Compute the time remaining to wait. tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;
	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

