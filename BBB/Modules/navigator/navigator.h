/*
 * navigator.h
 *
 *  Created on: Jul 11, 2014
 *      Author: martin
 */

#ifndef NAVIGATOR_H_
#define NAVIGATOR_H_

#include "behavior/behavior_enums.h"

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);

//#define RAD_TO_DEG (180.0 / M_PI)
//#define DEG_TO_RAD (M_PI / 180.0)
#define NORMALIZE_HEADING(x) (x + 360)%360

#define FIDO_RADIUS         		24.0f               //cm

#define DEGREESTORADIANS(x) ((x) * M_PI / 180.0)
#define RADIANSTODEGREES(x) (((x) * 180.0) / M_PI)

//reporting criteria
#define REPORT_BEARING_CHANGE 		2			//degrees
#define REPORT_CONFIDENCE_CHANGE 	0.2f	//probability
#define REPORT_MAX_INTERVAL			5			//seconds
#define REPORT_MIN_CONFIDENCE		0.5f
#define REPORT_LOCATION_CHANGE 		5

#define RAW_DATA_TIMEOUT 		5	//seconds
#define GPS_STABILITY_TIME 		30	//seconds
#define GPS_FIX_LOST_TIMEOUT	5	//seconds

//start navigator task
int NavigatorInit();

void NavigatorProcessMessage(psMessage_t *msg);

ActionResult_enum NavigatorIsGoodPose(bool waitForFix);

#endif
