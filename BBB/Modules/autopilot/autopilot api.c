/*
 ============================================================================
 Name        : autopilot_api.c
 Author      : Martin
 Version     :
 Copyright   : (c) 2015 Martin Lane-Smith
 Description : Receives MOVE commands
 ============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stropts.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>

#include "SoftwareProfile.h"
#include "PubSubData.h"
#include "pubsub/pubsub.h"
#include "pubsub/notifications.h"

#include "syslog/syslog.h"

#include "navigator/navigator.h"
#include "planner.h"
#include "autopilot.h"
#include "autopilot_common.h"
#include "Waypoints.h"


