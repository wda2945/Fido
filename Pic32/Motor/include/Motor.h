/* 
 * File:   Motor.h
 * Author: martin
 *
 * Created on January 14, 2014, 12:11 PM
 */


#ifndef MOTOR_H
#define	MOTOR_H

#include <stdint.h>
#include "Messages.h"

//Defines a common struct used by all motor tasks

//motor speed averaging
#define SAMPLE_COUNT 1

//direction (relative to chassis motion)
typedef enum {MOTOR_FORWARD, MOTOR_REVERSE} MotorDirectionEnum_t;
typedef enum {PORT_MOTORS, STARBOARD_MOTORS} MotorSideEnum_t;

//motor data table struct
typedef struct {
    //static data
    char                    *name;      // eg "Left Front"
    int                     side;       // MotorSideEnum_t
    
    //Encoder vars - interrupt updated
    volatile uint8_t        lastEncoderState; //used to decode quadrature encs
    volatile int64_t        encoderCount;   //inc/dec by interrupt
    volatile int32_t        encoderMiss;    //count of missed interrupts

    //encoder derived to calculate speed
    int64_t                 lastEncoderCount;
    TickType_t              lastCountTime;

    //speed averaging
    int64_t                 encoderChange[SAMPLE_COUNT];    //multiple samples for averaging
    int                     encoderSampleIndex;

    //encoder derived output
    float                   measuredSpeed;      //signed

    //current - not yet used
    volatile float          amps;
    float                   ampsZero;       //calibrated zero point

    //goals
    float                   distanceToGo;       //signed
    int                     desiredSpeed;       //signed
    
    //drive
    bool                    motorRunning;
    float                   currentDutyRatio;   //unsigned

    //PID weighted errors - not yet used
    float   pError, iError, dError;
    //PID workings
    float                   lastError;          //for differential term
    float                   errorIntegral;      //for integral term

} MotorStruct_t;

//data for all motors
extern MotorStruct_t    motors[NUM_MOTORS];

//motor side table struct
typedef struct {
    //static data
    char                    *name;      // eg "Left/Right"

    //encoder derived output
    float                   measuredSpeed;      //signed

    //goals
    float                   distanceToGo;       //signed
    int                     desiredSpeed;       //signed
    
    //drive
    MotorDirectionEnum_t    direction;
} MotorSideStruct_t;

extern MotorSideStruct_t motorSide[2];

//count of motors running - for AllStopped message
extern int motorsRunning;
extern bool motorsInhibit;

//PWM frequency
#define PWM_FREQUENCY       10000   //10kHz

#define FIDO_RADIUS         24.0f               //cm

#define MOVE_PER_COUNT      0.05f              //cm
#define COUNTS_PER_CM       20

#define RADIANS_TO_DEGREES (180.0f / M_PI)

//MOT Task message hook
bool MotTaskProcessMessage(psMessage_t *msg, TickType_t wait);
bool PidProcessMessage(psMessage_t *msg, TickType_t wait);

//task init Prototypes
int PidInit(); //Controls the motor PWM
int EncoderInit(); //Measures motor speed
int AmpsInit(); //Mesures motor current

extern PowerState_enum powerState;

#endif	/* MOTOR_H */

