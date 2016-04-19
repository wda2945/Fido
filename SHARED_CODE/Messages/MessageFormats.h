/* 
 * File:   MessageFormats.h
 * Author: martin
 *
 * Created on December 8, 2013, 10:53 AM
 */

#ifndef MESSAGEFORMATS_H
#define	MESSAGEFORMATS_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

#ifdef __XC32
#include "FreeRTOS.h"
#include "queue.h"
#else
typedef uint32_t TickType_t;
#endif

#include "messageenums.h"
#include "notificationenums.h"

//Common header for all messages

typedef struct {
    uint8_t length;         ///< Length of payload
    uint8_t source;         ///< ID of message sender (Subsystem_enum)
    uint8_t messageType;    ///< ID of message in payload (psMessageType_enum)
    uint8_t messageTopic;   ///< destination topic
} psMessageHeader_t;

#define SIZEOF_HEADER   4
#define SIZEOF_PADDING  0
#define PS_NAME_LENGTH 30
#define PS_MAX_LOG_TEXT 31
#define PS_SHORT_NAME_LENGTH 16

//----------------------PS_SYSLOG

typedef struct {
    uint8_t severity;
    char text[PS_MAX_LOG_TEXT+1];
} psLogPayload_t;

//----------------------BBB_SYSLOG,

#define BBB_MAX_LOG_TEXT 64

typedef struct {
    uint8_t severity;
    char file[5];
    char text[BBB_MAX_LOG_TEXT];
} bbbLogPayload_t;

//----------------------SYSTEM MANAGEMENT

//PS_REQUEST                        //PING

typedef struct {
    uint8_t responder;
    uint8_t requestor;
    long int currentTime;
} psRequestPayload_t;

//PS_RESPONSE

typedef struct {
    uint8_t requestor;
    uint8_t flags;					//pingResponseStatus_enum
    char subsystem[3];				//eg "MCP"
//    char name[PS_NAME_LENGTH+1];	//state info
} psResponsePayload_t;

//PS_TICK_PAYLOAD

//Data broadcast by 1S tick
typedef struct {
    uint8_t systemPowerState;		//PowerState_enum
} psTickPayload_t;

//PS_MASK
#define MASK_PAYLOAD_COUNT 2
typedef struct {
    NotificationMask_t value[MASK_PAYLOAD_COUNT];
    NotificationMask_t valid[MASK_PAYLOAD_COUNT];
}psMaskPayload_t;

//----------------------GENERAL PURPOSE FORMATS
//PS_BYTE

typedef struct {
    uint8_t value;
} psBytePayload_t;

//PS_INT

typedef struct {
    int32_t value;
} psIntPayload_t;

//PS_FLOAT

typedef struct {
    float value;
} psFloatPayload_t;

//PS_2FLOAT

typedef struct {
    union {
        struct {
            float x, y;
        };
        struct {
            float value1, value2;
        };
    };
} ps2FloatPayload_t;

//PS_3FLOAT

typedef struct {
    union {
        struct {
            float x, y, z;
        };
        struct {
            float value1, value2, value3;
        };
        struct {
            float xSpeed;
            float ySpeed;
            float zRotateSpeed;
         };
        struct {
        	float heading;
        	float pitch;
        	float roll;
        };
    };
} ps3FloatPayload_t;

//PS_NAME

typedef struct {
    char name[PS_NAME_LENGTH+1];
} psNamePayload_t;

//PS_NAMED_BYTE

typedef struct {
    uint8_t value;
    char name[PS_NAME_LENGTH+1];
} psNameBytePayload_t;

//PS_NAMED_FLOAT

typedef struct {
    float value;
    char name[PS_NAME_LENGTH+1];
} psNameFloatPayload_t;

//PS_NAMED_INT

typedef struct {
    int value;
    char name[PS_NAME_LENGTH+1];
} psNameIntPayload_t;

//PS_NAMED_3FLOAT

typedef struct {

    union {

        struct {
            float float1;
            float float2;
            float float3;
        };

        struct {
            float min;
            float max;
            float value;
        };
    };
    char name[PS_NAME_LENGTH+1];
} psName3FloatPayload_t;

//PS_NAMED_3INT

typedef struct {
    union {
        struct {
            int int1;
            int int2;
            int int3;
        };
        struct {
            int min;
            int max;
            int value;
        };
    };
    char name[PS_NAME_LENGTH+1];
} psName3IntPayload_t;

//PS_NAMED_4BYTE

typedef struct {
    union {
    uint8_t uint8[4];
    uint16_t uint16[2];
    uint32_t uint32;
    };
    char name[PS_NAME_LENGTH+1];
} psName4BytePayload_t;

//----------------------- CONFIG ---------------------------------------

//PS_CONFIG					//CONFIG & CONFIG_DONE

typedef struct {
	uint8_t responder;
	uint8_t requestor;
	uint8_t	count;			//CONFIG_DONE only
}psConfigPayload_t;

//PS_SETTING				//SETTING and NEW_SETTING

typedef struct {
	float min;
 	float max;
  	float value;
  	uint8_t subsystem;      //requestor for SETTING
  	char name[PS_NAME_LENGTH+1];
} psSettingPayload_t;

//PS_OPTION					//OPTION & CHANGE_OPTION

typedef struct {
	int min;
 	int max;
  	int value;
  	uint8_t subsystem;      //requestor for OPTION
  	char name[PS_NAME_LENGTH+1];
} psOptionPayload_t;

//---------------------- STATS -----------------------------------------------
//PS_TASK_STATS_PAYLOAD

typedef struct {
    uint16_t stackHeadroom;
    uint8_t percentCPU;
    char taskName[PS_NAME_LENGTH+1];
}psTaskStatsPayload_t;

//PS_SYS_STATS_PAYLOAD

typedef struct {
    uint32_t freeHeap;
    uint8_t     cpuPercentage;
} psSysStatsPayload_t;

//PS_COMMS_STATS_PAYLOAD

typedef struct {
    char destination[4];
    int messagesSent;
    int sendErrors;
    int addressDiscarded;
    int congestionDiscarded;      //congestion
    int logMessagesDiscarded;
    int messagesReceived;
    int receiveErrors;
    int addressIgnored;        //wrong address
    int parseErrors;
} psCommsStatsPayload_t;

//----------------------NAVIGATION & SENSORS------------------------------------------
//----------------------RAW

//PS_POSITION_REPORT

typedef struct {
	float latitude, longitude;
    float HDOP;
    uint8_t gpsStatus;
//    uint8_t hour, minute, seconds, year, month, day;
} psPositionPayload_t;

//PS_IMU

typedef struct {
    int16_t magX, 	magY, 	magZ;
    int16_t accelX,	accelY,	accelZ;
    int16_t gyroX, 	gyroY, 	gyroZ;
    struct timeval timestamp;
} psImuPayload_t;

//PS_ODOMETRY

typedef struct {
    //movement since last message
    float portMovement;         	// cm
    float starboardMovement;		// cm
    //current speed
    int16_t portSpeed;       		//cm per sec
    int16_t starboardSpeed;  		//cm per sec
    //current goal
    int16_t portMotorsToGo;          //cm to go
    int16_t starboardMotorsToGo;     //cm
    //status
    uint8_t motorsRunning;
} psOdometryPayload_t;

//------------------------COOKED

typedef struct {
	uint16_t heading;	//degrees
	int16_t pitch;		//degrees
	int16_t roll;		//degrees
} Orientation_struct;

typedef struct {
	float latitude;
	float longitude;
} Position_struct;

//PS_POSE_PAYLOAD

typedef struct {
	Position_struct position;
	Orientation_struct orientation;
	uint8_t orientationValid;
	uint8_t positionValid;
} psPosePayload_t;

//---------------------------------------------PROXIMITY----------------------------

//Detailed report
typedef struct {
    uint8_t direction;      //psProxDirection_enum (45 degree sectors clockwise from heading)
    uint8_t rangeReported;  //0 - 200 cm
    uint8_t confidence;     //0 - 100%
    uint8_t status;	
} psProxReportPayload_t;

//summary
typedef struct {       //bitmaps for 8 directions
	uint8_t proxStatus[PROX_SECTORS];		//ProxStatus_enum
} psProxSummaryPayload_t;

//----------------------MOVEMENT

//PS_MOTOR_PAYLOAT

typedef struct {
    int16_t portMotors;         //cm
    int16_t starboardMotors;    //cm
    uint8_t speed;              //cm per sec
    uint8_t flags;              //MotorFlags_enum
} psMotorPayload_t;

//----------------------ENVIRONMENT

//PS_BATTERY
typedef struct {
    uint16_t volts;			//times 10
    int16_t ampHours;		//times 10
    int16_t amps;			//times 10
    uint8_t percentage;
    uint8_t status;			//BatteryStatus_enum
} psBatteryPayload_t;

//PS_ENVIRONMENT - MCP
typedef struct {
    uint16_t batteryVolts;   //times 10
    int16_t  batteryAmps;    //times 10
    int16_t  batteryAh;      //times 10
    uint16_t solarVolts;     //times 10
    uint16_t chargeVolts;    //times 10
    uint8_t  solarAmps;      //times 10
    uint8_t  chargeAmps;     //times 10
    uint8_t  internalTemp;
    uint8_t  externalTemp;
    uint8_t  relativeHumidity;
    uint8_t  ambientLight;
    uint8_t  isRaining;     //boolean
} psEnvironmentPayload_t;

//PS_BEHAVIOR_STATUS

typedef struct {
    int status;
    char behavior[PS_SHORT_NAME_LENGTH];
    char lastLuaCallFail[PS_SHORT_NAME_LENGTH];
    char lastFailReason[PS_SHORT_NAME_LENGTH];
} psBehaviorStatusPayload_t;


#endif	/* MESSAGEFORMATS_H */

