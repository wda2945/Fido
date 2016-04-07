/*
 ============================================================================
 Name        : scanner.c
 Author      : Martin
 Version     :
 Copyright   : (c) 2013 Martin Lane-Smith
 Description : Receives FOCUS messags from the overmind and directs the scanner accordingly.
 	 	 	   Receives proximity detector messages and publishes a consolidated PROX-REP
 ============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stropts.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>

#include "pwm.h"
#include "gpio.h"
#include "i2c.h"

#include "softwareprofile.h"

#include "pubsubdata.h"
#include "pubsub/pubsub.h"

#include "syslog/syslog.h"

#include "scanner.h"

FILE *scanDebugFile;

#ifdef SCANNER_DEBUG
#define DEBUGPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(scanDebugFile, __VA_ARGS__);fflush(scanDebugFile);
#else
#define DEBUGPRINT(...) fprintf(scanDebugFile, __VA_ARGS__);fflush(scanDebugFile);
#endif

#define ERRORPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(scanDebugFile, __VA_ARGS__);fflush(scanDebugFile);

typedef uint8_t PROX_BITMAP;

//LIDAR scanning parameters
#define SCAN_LIMIT				60
#define LEFT_SCAN_LIMIT 		-SCAN_LIMIT			//120 degree field
#define RIGHT_SCAN_LIMIT		SCAN_LIMIT
#define SCAN_STEP			  	5
#define SCAN_COUNT				((RIGHT_SCAN_LIMIT - LEFT_SCAN_LIMIT) / SCAN_STEP)
#define SCANS_PER_SECTOR		((360 / PROX_SECTORS) / SCAN_STEP)
#define STEPS_PER_PASS			(SCAN_COUNT / 3)
#define MAX_TIME_TO_SCAN		10			//max seconds between scans of any direction

//range processing parameters
#define MIN_PROX_RANGE	20				//cm - no data closer
#define MAX_PROX_RANGE 200				//cm - ignore obstacles further
#define RANGE_BUCKETS	20				//[<=20cm | <18 x 10cm> | >200cm]
#define RANGE_BUCKET_SIZE ((MAX_PROX_RANGE-MIN_PROX_RANGE)/(RANGE_BUCKETS-2))	//10cm
#define PRIOR_PROBABILITY		0.1f	//10% chance of an existing obstacle, in absence of data
#define MIN_CONFIDENCE			0.5f	//require >50% to declare an obstacle present
#define NEAR_PROXIMITY			3		//first 3 buckets = near (<~40cm)
#define CONFIDENCE_CHANGE		0.2f	//confidence change to be reported

//request from the brain
int focusHeading = 0;

//deriveD servo parameters
#define SERVO_PERIOD			(1000.0/SERVO_FREQUENCY)	//ms

#define DUTY_CYCLE(angle)		((angle>=0 ? \
		(MID_PULSE + angle*(MAX_PULSE-MID_PULSE)/SCAN_LIMIT) : \
		(MID_PULSE - angle*(MID_PULSE-MIN_PULSE)/SCAN_LIMIT))	/ SERVO_PERIOD)

//convenience macro
#define SECTOR_WIDTH (360 / PROX_SECTORS)
#define SectorFromRelativeBearing(servoangle)  (servoangle>=0 ? \
		((servoangle+SECTOR_WIDTH/2)/SECTOR_WIDTH) : \
		((servoangle-SECTOR_WIDTH/2)/SECTOR_WIDTH) + 8 )

static const float RelativeBearingFromSector[PROX_SECTORS] =
{0.0, 45.0, 90.0, 135.0, 180.0, -135, -90.0, -45.0};

#define LEFT_SCAN_SECTOR (SectorFromRelativeBearing(LEFT_SCAN_LIMIT))
#define RIGHT_SCAN_SECTOR (SectorFromRelativeBearing(RIGHT_SCAN_LIMIT))

psProxSummaryPayload_t proxSummary;

//8 Proximity sectors
typedef struct  {
	PROX_BITMAP	reportingMask;
	bool report;				//send report
	ProxStatus_enum status;			//whether obstacle thought present
	float confidence;			//0 - 1
	int rangeBucket;			//0 to RANGE_BUCKETS-1
	int rangeReported;
} ProxSectorData_t;
ProxSectorData_t proxSectorData[PROX_SECTORS];

typedef struct {
	int servoAngle;				//angle in degrees
	int proxSectorNumber;		//index into proxSectorData
	bool scanNext;				//scan in upcoming pass
	time_t lastScanned;			//wall clock time in seconds
	int errors;
	float weight;
	float probObstacle[RANGE_BUCKETS];
	float maxProb;
	int maxProbRangeBucket;
}ScanData_t;
ScanData_t scanData[SCAN_COUNT];

#define PROXIMITY_CONTACT_WEIGHTING  	3
#define PROXIMITY_CLOSE_WEIGHTING		2
#define PROXIMITY_FAR_WEIGHTING		1

//add observation to database
void Update(int scanStep, int rangeReported);

//normalize sector histogram
void Normalize(int scanStep);

//calculate likelyhood
void calcProximity();

//I2C LIDAR interfacing
char I2CdevName[80];   //LIDAR I2C path

//start and read LIDAR commands
int StartRanging();
int ReadRange();

PwmChannel_t servo;		//servo open file numbers

//thread to manage scanner
void *ScannerThread(void *arg);

pthread_t ScannerInit()
{
	int r, s;

	scanDebugFile = fopen("/root/logfiles/scanner.log", "w");

	//start PWM - should central the servo

	if (pwm_start(SERVO_PWM_KEY, SERVO_PWM_EXPORT, SERVO_PWM_DIR, DUTY_CYCLE(0), SERVO_FREQUENCY, 0, &servo) != 0)
	{
		ERRORPRINT("Servo pwm_start failed\n");
		return -1;
	}

	//start I2C
	if (i2c_setup(LIDAR_SCL_PIN, LIDAR_SDA_PIN) < 0)
	{
		ERRORPRINT("LIDAR I2C setup fail\n");
		return -1;
	}

	//LIDAR I2C device path
	snprintf(I2CdevName, sizeof(I2CdevName), "/dev/i2c-%d", LIDAR_I2C);

	//initialize Proximity data
	for (s=0; s<PROX_SECTORS; s++)
	{
		proxSectorData[s].reportingMask = 0x1 << s;
		proxSectorData[s].report = false;
		proxSectorData[s].status = 0;
		proxSectorData[s].confidence = 0.0f;
		proxSectorData[s].rangeBucket = -1;
	}
	//initialize Scan data
	time_t now = time(NULL);
	for (s=0; s<SCAN_COUNT; s++)
	{
		for (r=0; r<RANGE_BUCKETS; r++)
		{
			scanData[s].probObstacle[r] = 0.0f;
		}
		scanData[s].servoAngle	= LEFT_SCAN_LIMIT + (s * SCAN_STEP);
		scanData[s].proxSectorNumber = SectorFromRelativeBearing(scanData[s].servoAngle);
		scanData[s].scanNext = false;
		scanData[s].lastScanned = now;
		scanData[s].weight = 0;
	}

	//create scanner thread
	pthread_t thread;
	int result = pthread_create(&thread, NULL, ScannerThread, NULL);
	if (result != 0)
	{
		ERRORPRINT("No Scanner thread");
		return -1;
	}

	return thread;
}

int LIDAR_FD;

void *ScannerThread(void *arg)
{
	struct timespec request, remain;
	int scanLast = 0;
	int servoDirection = 1;	//1 = left to right

	int scanStep;
	int proxSector;

	int reply;
	int a, r, i;

	if ((LIDAR_FD = open(I2CdevName, O_RDWR)) < 0){
		ERRORPRINT("open(%s) fail - %s\n", I2CdevName, strerror(errno));
		return 0;
	}
	if (ioctl(LIDAR_FD, I2C_SLAVE, LIDAR_I2C_ADDRESS) < 0){
		ERRORPRINT("I2C Addr %2x failed: %s\n", LIDAR_I2C_ADDRESS, strerror(errno) );
		return 0;
	}

	while(1)
	{
		//recalculate weighting/priority factors for each scan step
		for (scanStep=0; scanStep<SCAN_COUNT; scanStep++)
		{
			proxSector = scanData[scanStep].proxSectorNumber;
			//weighting factors
			//what report have we already?
			ProxStatus_enum currentStatus = proxSectorData[proxSector].status;
			int proximityFactor;

			if (currentStatus == PROX_CONTACT) proximityFactor = PROXIMITY_CONTACT_WEIGHTING;
			else if (currentStatus == PROX_CLOSE) proximityFactor = PROXIMITY_CLOSE_WEIGHTING;
			else if (currentStatus == PROX_FAR) proximityFactor = PROXIMITY_FAR_WEIGHTING;
			else proximityFactor = 0;

			//how close to our direction of focus?
			int nearFocus = abs(scanData[scanStep].servoAngle - focusHeading);

			//how near the front?
			int forwardFacing = abs(scanData[scanStep].servoAngle);

			//how long since we last scanned?
			int sampleAge = time(NULL) - scanData[scanStep].lastScanned;

			//proximity reports
			PROX_BITMAP mask = proxSectorData[proxSector].reportingMask;

//			scanData[scanStep].weight = proximityFactor * proxWeight +
//					nearFocus * focusWeight +
//					forwardFacing * fwdWeight +
//					sampleAge * ageWeight
//					+((mask & proxSummary.contact) ? contactWeight : 0)
//					+((mask & proxSummary.close) ? closeWeight : 0)
//					+((mask & proxSummary.distant) ? distantWeight : 0)
					;

			scanData[scanStep].scanNext = true;
		}

		//find the top priorities
		int sectorsToScan = 0;
		float maxWeight;
		int nextScanStep;
		while (sectorsToScan < STEPS_PER_PASS)
		{
			maxWeight = -1;
			nextScanStep = -1;
			for (scanStep=0; scanStep<SCAN_COUNT; scanStep++)
			{
				if (scanData[scanStep].weight > maxWeight && !scanData[scanStep].scanNext)
				{
					maxWeight = scanData[scanStep].weight;
					nextScanStep = scanStep;
				}
			}
			scanData[nextScanStep].scanNext = true;
			sectorsToScan++;
		}

		//now do the scan
		if (servoDirection > 0)
		{
			//left to right
			scanStep = 0;
		}
		else
		{
			//right to left
			scanStep = SCAN_COUNT-1;
		}

		while (scanStep >= 0 && scanStep < SCAN_COUNT)
		{

			if (scanData[scanStep].scanNext)
			{
				//move servo
				int angle = scanData[scanStep].servoAngle;

				int pulse = 1000 + 1000 * (angle + 90) / 180;	//uSec
				float dutycycle = pulse / (1000000 / SERVO_FREQUENCY);
//				DEBUGPRINT("Angle: %i, pulse: %i, duty: %f\n", angle, pulse, dutycycle);

				pwm_set_duty_cycle(&servo, dutycycle);
//				pwm_set_duty_cycle(&servo, 0);

				int delay = MIN_MOVE_MS + MS_PER_STEP * abs(scanStep - scanLast); //millisecs

				//sleep while the servo moves
				request.tv_sec = 0;
				request.tv_nsec = delay * 1000000;
				reply = nanosleep(&request, &remain);

				//take range reading
				StartRanging();

				//sleep while ranging
//				request.tv_sec = 0;
//				request.tv_nsec = 100 * 1000000;	//50 millisecs
//				reply = nanosleep(&request, &remain);

				sleep(1);

				int range = ReadRange();

				sleep(1);

				//process range data
				//				if (range > 0)
				//				{
				scanLast = scanStep;
				//					Update(servoCurrent, range);
				//				}
				//				else
				//				{
				//					scanData[scanStep].errors++;
				//				}
			}
			scanStep += servoDirection;
		}
		servoDirection = -servoDirection;	//switch direction

		//calculate proximity and report
		//		calcProximity(i);
		sleep(1);
	}
}

int StartRanging()
{
	struct timespec request, remain;
	unsigned char rangeCommand[2] = {RegisterMeasure, MeasureValue};

	int bytesWritten = write(LIDAR_FD, &rangeCommand, 2);
	if (bytesWritten == -1){
		ERRORPRINT("Write Range Command error: %s\n",strerror(errno) );
		return -1;
	}
	DEBUGPRINT("LIDAR Start Ranging\n");
	return 0;
}

//read range from LIDAR
int ReadRange()
{
	/* Using SMBus commands */
//	int res = i2c_smbus_read_word_data(LIDAR_FD, RegisterHighLowB);
//	if (res < 0) {
//		DEBUGPRINT("i2c_smbus_read_word_data error: %s\n",strerror(errno) );
//		return -1;
//	} else {
//	//byte swap
//	int range = ((res & 0xff) << 8) | ((res & 0xff00) >> 8);
//	DEBUGPRINT("LIDAR Range = %i\n", range);
//	return range;
//}

	unsigned char readCommand[1] = {RegisterHighLowB};
	unsigned char readData[2];

	int bytesWritten = write(LIDAR_FD, &readCommand, 1);
	if (bytesWritten == -1){
		ERRORPRINT("Write ReadReg Command error: %s\n",strerror(errno) );
		return -1;
	}

	int bytesRead = read(LIDAR_FD, &readData, 2);
	if (bytesRead != 2){
		ERRORPRINT("ReadReg error: %s\n",strerror(errno) );
		return -1;
	}
	else
	{
		int range = ((readData[0] & 0xff) << 8) | ((readData[1] & 0xff));
		DEBUGPRINT("LIDAR Range = %i\n", range);
		return range;
	}


}

//Bayes code
void Update(int s, int rangeReported)
{
	DEBUGPRINT("Update: Range %i at &i\n", rangeReported, scanData[s].servoAngle);

	int r;
	//calculate likelihood
	float range = (float) rangeReported;

	for (r = 0; r<RANGE_BUCKETS; r++)
	{
		float p;
		float bucketCenter = (float) (r + 0.5) * RANGE_BUCKET_SIZE + MIN_PROX_RANGE;
		float falseNegative = (r / RANGE_BUCKETS) * (LIDAR_FALSE_NEGATIVE_200 - LIDAR_FALSE_NEGATIVE_20) + LIDAR_FALSE_NEGATIVE_20;

		if (r == 0 && rangeReported <= MIN_PROX_RANGE)
		{
			//minimum range case
			p = (1 - LIDAR_FALSE_POSITIVE);
		}
		else if (r == RANGE_BUCKETS-1 && rangeReported >= bucketCenter)
		{
			//maximum range case
			p = (1 - LIDAR_FALSE_POSITIVE);
		}
		else
		{
			if (fabs(bucketCenter - range) < RANGE_BUCKET_SIZE/2)
			{
				//hit
				p = (1 - LIDAR_FALSE_POSITIVE);
			}
			else
			{
				if (bucketCenter < range)
				{
					//nearer buckets - false negatives?
					p = falseNegative / s;
				}
				else
				{
					//further buckets - shadowed by false positive?
					p = LIDAR_FALSE_POSITIVE / (RANGE_BUCKETS - s);
				}
			}

		}

		//CALCULATE the likelihood across the affected sectors
		scanData[s].probObstacle[r] *= p;
	}
}

void Normalize(int s)
{
	int r;
	float total = 0.0;
	for (r = 0; r<RANGE_BUCKETS; r++)
	{
		total += scanData[s].probObstacle[r];
	}
	for (r = 0; r<RANGE_BUCKETS; r++)
	{
		scanData[s].probObstacle[r] /= total;
	}
}

void calcProximity()
{
	//determine obstacle presence and confidence for a sector
	int r, s, sector, scanStep;
	float invProbObstacle[RANGE_BUCKETS];

	ProxStatus_enum oldStatus;
	float oldConfidence;

	for (sector=LEFT_SCAN_SECTOR; sector!=RIGHT_SCAN_SECTOR; sector = (sector+1) % PROX_SECTORS)
	{
		oldStatus = proxSectorData[sector].status;
		oldConfidence = proxSectorData[sector].confidence;

		for (r=0; r<RANGE_BUCKETS; r++)
		{
			invProbObstacle[r] = 1.0f;	//inverse probability (1 - prob)
		}

		for (scanStep=0; scanStep<SCAN_COUNT; scanStep++)
		{
			if (scanData[s].proxSectorNumber == sector)
			{
				Normalize(s);
				for (r = 0; r<RANGE_BUCKETS-1; r++)
				{
					invProbObstacle[r] *= (1.0f - scanData[s].probObstacle[r]);		//calc joint probabliity
				}

			}
		}

		//find highest probability range bucket
		float minInvProb = 2.0f;
		int maxProbRangeBucket = -1;
		for (r = 0; r<RANGE_BUCKETS-1; r++)
		{
			if ( invProbObstacle[r] < minInvProb)
			{
				minInvProb = invProbObstacle[r];
				maxProbRangeBucket = r;
			}
		}
		float maxProb = 1.0f - minInvProb;
		//enough for a hit?
		if (maxProb > MIN_CONFIDENCE)
		{
			proxSectorData[sector].confidence = maxProb;
			proxSectorData[sector].rangeBucket = maxProbRangeBucket;
			if (maxProbRangeBucket < NEAR_PROXIMITY)
			{
				proxSectorData[sector].status = PROX_CLOSE;
			}
			else
			{
				proxSectorData[sector].status = PROX_FAR;
			}
		}
		else
		{
			proxSectorData[sector].confidence = 1 - maxProb;
			proxSectorData[sector].rangeBucket = RANGE_BUCKETS-1;
			proxSectorData[sector].status = 0;
		}
		proxSectorData[sector].rangeReported = (int)((float)(proxSectorData[sector].rangeBucket + 0.5) * RANGE_BUCKET_SIZE + MIN_PROX_RANGE);

		if (proxSectorData[sector].status != oldStatus
				|| fabs(proxSectorData[sector].confidence - oldConfidence) > CONFIDENCE_CHANGE)
		{
//			psMessage_t msg;
//			psInitPublish(msg, TARGETREP);
//			msg.proxReportPayload.direction 	= sector;
//			msg.proxReportPayload.confidence 	= (proxSectorData[sector].confidence * 100);
//			msg.proxReportPayload.rangeReported	= proxSectorData[sector].rangeReported;
//			msg.proxReportPayload.status		= proxSectorData[sector].status;
//			//		NewBrokerMessage(&msg);
//			DEBUGPRINT("TARGETREP sector=%i, range=%i\n", sector, msg.proxReportPayload.rangeReported);
		}
	}
}
//callback to monitor messages
void ScannerProcessMessage(psMessage_t *msg)
{
//	DEBUGPRINT("Scanner: %s\n", psLongMsgNames[msg->header.messageType]);

	switch (msg->header.messageType)
	{
	case PROXREP:
		proxSummary = msg->proxSummaryPayload;
		break;
	default:
		break;
	}
}

//Proximity status call from Behavior Tree
ActionResult_enum proximityStatus(ProxSectorMask_enum _sectors,  ProxStatusMask_enum _status)
{
	int i;
	for (i=0; i<PROX_SECTORS; i++)
	{
		ProxSectorMask_enum sectorMask = 0x1 << i;
		if ((sectorMask & _sectors))
		{
			//wanted direction
			ProxStatusMask_enum statusMask = 0x1 << proxSummary.proxStatus[i];
			if ((statusMask & _status))
			{
				//wanted status
				return ACTION_SUCCESS;
			}
		}
	}
	return ACTION_FAIL;
}
