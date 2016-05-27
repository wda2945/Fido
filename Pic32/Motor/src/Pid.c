/*
 * File:   PidTask.c
 * Author: martin
 *
 * Created on January 14, 2014
 */
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#define _SUPPRESS_PLIB_WARNING
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING

#include "plib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSub/Notifications.h"
#include "PubSubData.h"

#include "Motor.h"
#include "Drivers/Config/Config.h"

//main motor data struct
SemaphoreHandle_t motorStructMutex; //access control
MotorStruct_t motors[NUM_MOTORS];
MotorSideStruct_t motorSide[2];

MotorSideStruct_t *portMotors       = &motorSide[PORT_MOTORS];
MotorSideStruct_t *starboardMotors  = &motorSide[STARBOARD_MOTORS];

int motorsRunning           = 0;
bool motorsInhibit          = true;

float maxDistanceToGo       = 0.0;
float setSpeed              = 0.0;

uint8_t moveFlags           = 0;    //from the command - MotorFlags_enum

unsigned int timerPeriod;
TickType_t currentPidTimerInterval;

#define DECELERATION_STEPS  4

struct {
    float range;
    float factor; //0.0 - 1.0
} decelerationCurve[DECELERATION_STEPS] = {
    {5, 0.0},
    {50, 0.75},
    {100, 1.0},
    {200, 1.0},
};

//list of connectors in motor order
int motorToConnector[4] = MOTORS_TO_CONNECTOR;
//list of OC channels in motor order
int motorToOutCompare[4] = MOTORS_TO_OC;

//Functions to implement the motor control PID loop
//Executes periodically to determine speed error and adjust PWM ratio

//PID task
static void PIDTask(void *pvParameters);

//utility functions to change motor speed and direction
float setDutyCycle(int motor, float duty); //duty is +/- forward and reverse
void setDirection(int motor, MotorDirectionEnum_t direction);

//init called once from MotorTask

int PidInit() {
    int i;

    //mutex to allow clean updates from messages
    motorStructMutex = xSemaphoreCreateMutex();
    while (motorStructMutex == NULL) {
        LogError("Pid Motor Goals Mutex");
        SetCondition(MOT_INIT_ERROR);
        return -1;
    }

    //init motor struct
    motors[FRONT_LEFT_MOTOR].name   = "FL";
    motors[FRONT_RIGHT_MOTOR].name  = "FR";
    motors[REAR_LEFT_MOTOR].name    = "RL";
    motors[REAR_RIGHT_MOTOR].name   = "RR";
    motors[FRONT_LEFT_MOTOR].side   = PORT_MOTORS;
    motors[FRONT_RIGHT_MOTOR].side  = STARBOARD_MOTORS;
    motors[REAR_LEFT_MOTOR].side    = PORT_MOTORS;
    motors[REAR_RIGHT_MOTOR].side   = STARBOARD_MOTORS;

    for (i = 0; i < 4; i++) {
        motors[i].encoderCount = 0UL;
        motors[i].lastEncoderCount = 0UL;
        motors[i].encoderMiss = 0;

        motors[i].desiredSpeed = 0;
        motors[i].measuredSpeed = 0;

        motors[i].motorRunning = false;
        motors[i].currentDutyRatio = 0.0f;

        motors[i].pError = motors[i].iError = motors[i].dError = 0.0f;
        motors[i].lastError = 0;
        motors[i].errorIntegral = 0;
        setDirection(i, MOTOR_FORWARD);
    }

    for (i = 0; i < 2; i++)
    {
        motorSide[i].desiredSpeed   = 0;
        motorSide[i].direction      = 0;
        motorSide[i].distanceToGo   = 0.0;
        motorSide[i].measuredSpeed  = 0.0;
    }
    portMotors->name        = "port";
    starboardMotors->name   = "starboard";
    
    //initialise timer 2 for PWM - not running for now
    timerPeriod = GetPeripheralClock() / PWM_FREQUENCY; //count using 1:1 prescaler frequency
    OpenTimer2(T2_OFF | T2_IDLE_CON | T2_PS_1_1 | T2_SOURCE_INT, timerPeriod);

    //Initialise OC peripherals
#define OC_CONFIG (OC_ON | OC_IDLE_CON | OC_TIMER_MODE16 | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE)
    OpenOC2(OC_CONFIG, timerPeriod + 1, timerPeriod + 1); //set to 100% = stop
    OpenOC3(OC_CONFIG, timerPeriod + 1, timerPeriod + 1);
    OpenOC4(OC_CONFIG, timerPeriod + 1, timerPeriod + 1);
    OpenOC5(OC_CONFIG, timerPeriod + 1, timerPeriod + 1);

    //due to the 7405 inverter, the PWM output is inverted. 0% is max speed and 100% is stop!

    //TODO: stagger PWM pulses
    //TODO: use OC interrupts to trigger Amps ADC conversion

    //start Timer 2
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_PS_1_64 | T2_SOURCE_INT, timerPeriod);

    PORTClearBits(MOT_RST_IOPORT, MOT_RST_BIT); //put drivers out of reset

    //start the PID task
    if (xTaskCreate(PIDTask, /* The function that implements the task. */
            (signed char *) "PID Task", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
            PID_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            PID_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL) /* The task handle is not required, so NULL is passed. */
            != pdPASS) {
        LogError("PID Task");
        SetCondition(MOT_INIT_ERROR);
        return -1;
    }

    return 0;
}

//emergency stop

void AbortMotors() {
    if (motorsRunning) {
//        CancelCondition(MOTORS_BUSY);

        int i;
        for (i = 0; i < 4; i++) {
            setDutyCycle(i, 0.0);
            motors[i].distanceToGo = 0;
            motors[i].desiredSpeed = 0;
//            motors[i].motorRunning = false;
        }
 //       motorsRunning                   = 0;
        
        portMotors->distanceToGo        = 0.0;
        starboardMotors->distanceToGo   = 0.0;
        maxDistanceToGo                 = 0.0;
        setSpeed                        = 0.0;
        portMotors->desiredSpeed        = 0.0;
        starboardMotors->desiredSpeed   = 0.0;
        
        DebugPrint("Abort Motors");
    }
}

//handle messages from elsewhere

bool PidProcessMessage(psMessage_t *msg, TickType_t wait) {

    int semHeld = xSemaphoreTake(motorStructMutex, 50);
    
    switch (msg->header.messageType) {
        case MOTORS:
        {
            DebugPrint("MOTORS Msg: %i, %i, speed=%i", msg->motorPayload.portMotors, 
                    msg->motorPayload.starboardMotors,
                    msg->motorPayload.speed);
            
            //motor messages from Overmind

            moveFlags = msg->motorPayload.flags;

            if (moveFlags & ADJUSTMENT) {
                //new goals to be added to current
                motors[FRONT_LEFT_MOTOR].distanceToGo += (float) msg->motorPayload.portMotors;
                motors[FRONT_RIGHT_MOTOR].distanceToGo += (float) msg->motorPayload.starboardMotors;
                motors[REAR_LEFT_MOTOR].distanceToGo += (float) msg->motorPayload.portMotors;
                motors[REAR_RIGHT_MOTOR].distanceToGo += (float) msg->motorPayload.starboardMotors;
            } else {
                motors[FRONT_LEFT_MOTOR].distanceToGo = (float) msg->motorPayload.portMotors;
                motors[FRONT_RIGHT_MOTOR].distanceToGo = (float) msg->motorPayload.starboardMotors;
                motors[REAR_LEFT_MOTOR].distanceToGo = (float) msg->motorPayload.portMotors;
                motors[REAR_RIGHT_MOTOR].distanceToGo = (float) msg->motorPayload.starboardMotors;
            }
            setSpeed = msg->motorPayload.speed * 50.0;     //unsigned

            portMotors->distanceToGo    = (motors[FRONT_LEFT_MOTOR].distanceToGo
                                + motors[REAR_LEFT_MOTOR].distanceToGo) / 2;        //signed
            starboardMotors->distanceToGo = (motors[FRONT_RIGHT_MOTOR].distanceToGo
                                    + motors[REAR_RIGHT_MOTOR].distanceToGo) / 2;
            maxDistanceToGo     = max((abs(portMotors->distanceToGo)), abs(starboardMotors->distanceToGo)); //..+ve

            if (maxDistanceToGo > 0 && setSpeed > 0) {
                portMotors->desiredSpeed = setSpeed * portMotors->distanceToGo / maxDistanceToGo;   //signed
                starboardMotors->desiredSpeed = setSpeed * starboardMotors->distanceToGo / maxDistanceToGo;
                
                if (portMotors->desiredSpeed >= 0) portMotors->direction = MOTOR_FORWARD;
                    else portMotors->direction = MOTOR_REVERSE;
                if (starboardMotors->desiredSpeed >= 0) starboardMotors->direction = MOTOR_FORWARD;
                    else starboardMotors->direction = MOTOR_REVERSE;
            } else {
                   DebugPrint("Stopping\n");
                   AbortMotors();
            }
        }
        break;
        case CONDITIONS:
            //conditions
            if (msg->maskPayload.valid[0] & NOTIFICATION_MASK(MOTORS_INHIBIT)) {        //assuming MOTORS_INHIBIT < 64
                if (msg->maskPayload.value[0] & NOTIFICATION_MASK(MOTORS_INHIBIT)) {
                    if (!motorsInhibit) {
                        motorsInhibit = true;
                        DebugPrint("Setting MOTORS_INHIBIT\n");
                        AbortMotors();
                    }
                } else {
                    if (motorsInhibit) {
                        motorsInhibit = false;
                        DebugPrint("Canceling MOTORS_INHIBIT\n");
                    }
                }
            }
            if ((msg->maskPayload.valid[0] & msg->maskPayload.value[0] & NOTIFICATION_MASK(BATTERY_CRITICAL)) //assuming BATERY_CRITICAL < 64
                    && (moveFlags & ENABLE_SYSTEM_ABORT)) {
                motorsInhibit = true;
                AbortMotors();
                LogInfo("Battery Halt");
            }
            break;
        case NOTIFY:
            //events
            switch (msg->intPayload.value) {
                case BATTERY_SHUTDOWN_EVENT:
                    motorsInhibit = true;
                    AbortMotors();
                    LogInfo("Battery Halt");
                    break;
                case SLEEPING_EVENT:
                    motorsInhibit = true;
                    AbortMotors();
                    LogInfo("Sleeping Halt");
                    break;
                case FRONT_CONTACT_EVENT:
                    DebugPrint("Notification: %s\n", eventNames[msg->intPayload.value]);
                    if (moveFlags & ENABLE_FRONT_CONTACT_ABORT) {
                        AbortMotors();
                        LogInfo("Front Contact Halt");
                    }
                    break;
                case REAR_CONTACT_EVENT:
                    DebugPrint("Notification: %s\n", eventNames[msg->intPayload.value]);
                    if (moveFlags & ENABLE_REAR_CONTACT_ABORT) {
                        AbortMotors();
                        LogInfo("Rear Contact Halt");
                    }
                    break;
                default:
                    break;
            }
            break;
    }
    if (semHeld == pdTRUE) xSemaphoreGive(motorStructMutex);

    return true;
}

#define MOTOR_TRACE(x,y) if (motorsRunning && i==0) {\
            DebugPrint("%s: %s = %0.1f, speed = %0.0f",thisMotor->name, x, y, thisMotor->measuredSpeed);\
        }
#define TRACE0(m) if (thisMotor->motorRunning && m==0) {\
            DebugPrint("%s: D=%0.0f, meas=%0.0f, desrd=%i, pwm=%0.2f",thisMotor->name,\
            thisMotor->distanceToGo, thisMotor->measuredSpeed,\
             thisMotor->desiredSpeed, thisMotor->currentDutyRatio);\
        }

//PID Task

static void PIDTask(void *pvParameters) {
    int i, j;
    MotorStruct_t *thisMotor;

    motorsRunning = 0; //count of motors running
    int motorsRunningReported = 0; //copy

    TickType_t now;
    int interval; //since last loop

    //odometry data

    int64_t portEncodersReported = 0;
    int64_t starboardEncodersReported = 0;


    float decelerationFactor = 1.0;

    TickType_t lastOdoReport = 0;
    psMessage_t odoMsg;

    //prep odometry message
    psInitPublish(odoMsg, ODOMETRY);

    TickType_t LastWakeTime = xTaskGetTickCount();

    for (;;) {

        int sample; //index into averaging array - not currently used
        int64_t totalChange; //average = totalChange / count

        //init vars for this loop
        //distance
        int64_t portEncoders = 0;
        int64_t starboardEncoders = 0; //totals for each side
        //speed
        float portSumSpeeds = 0;
        float starboardSumSpeeds = 0; //totals for each side

        PORTToggleBits(USER_LED_IOPORT, USER_LED_BIT);

        xSemaphoreTake(motorStructMutex, 50);

        //deceleration curve
        int j;
        maxDistanceToGo     = max((abs(portMotors->distanceToGo)), abs(starboardMotors->distanceToGo)); //..+ve
        decelerationFactor = 1.0;
        for (j = 0; j < DECELERATION_STEPS; j++) {
            if (maxDistanceToGo <= decelerationCurve[j].range) {
                decelerationFactor = decelerationCurve[j].factor;
                break;
            }
        }
        
        for (i = 0; i < 4; i++) {
            //for each motor
            
            thisMotor = &motors[i];

            //measure speed & distance, since last time, for this motor

            //index for averaging buffer - not in use
            sample = thisMotor->encoderSampleIndex;

            //change in encoder count since last check
            int64_t encoderChange = thisMotor->encoderChange[sample]
                    = (int64_t) (thisMotor->encoderCount - thisMotor->lastEncoderCount);
            thisMotor->lastEncoderCount = thisMotor->encoderCount;

            //time interval
            now = xTaskGetTickCount();
            interval = (int) (now - thisMotor->lastCountTime); //in mS
            thisMotor->lastCountTime = now;
            
            //average the speed (not in use)
            thisMotor->encoderSampleIndex = (sample + 1) % SAMPLE_COUNT;

            //average the last N changes
            totalChange = 0;
            for (j = 0; j < SAMPLE_COUNT; j++) {
                totalChange += thisMotor->encoderChange[j];
            }

            //average speed
            if (interval > 0) {
                thisMotor->measuredSpeed = (totalChange * 1000) / (SAMPLE_COUNT * interval); //counts per second
            } else {
                thisMotor->measuredSpeed = 0;
            }
            float travel = MOVE_PER_COUNT * totalChange / SAMPLE_COUNT;
            thisMotor->distanceToGo -= travel;

            switch (i) {
                case FRONT_LEFT_MOTOR:
                case REAR_LEFT_MOTOR:
                    //averaging port motors
                    portEncoders += encoderChange;
                    portMotors->distanceToGo -= travel / 2;
                    portSumSpeeds += thisMotor->measuredSpeed;
                    break;
                case FRONT_RIGHT_MOTOR:
                case REAR_RIGHT_MOTOR:
                    //averaging starboard motors
                    starboardEncoders += encoderChange;
                    starboardMotors->distanceToGo -= travel / 2;
                    starboardSumSpeeds += thisMotor->measuredSpeed;
                    break;
            }

            portMotors->measuredSpeed = (portSumSpeeds / 2);
            starboardMotors->measuredSpeed = (starboardSumSpeeds / 2);

            TRACE0(i);

            if (
                    ((motorSide[thisMotor->side].direction == MOTOR_FORWARD) && (thisMotor->distanceToGo <= 0))
                    || ((motorSide[thisMotor->side].direction == MOTOR_REVERSE) && (thisMotor->distanceToGo >= 0))
                    || (abs(thisMotor->distanceToGo) < 5)) {
                thisMotor->desiredSpeed = 0; //reached or passed goal
                thisMotor->distanceToGo = 0;
                if (thisMotor->motorRunning) {
                    DebugPrint("%s motor reached goal", thisMotor->name);
                    float result = setDutyCycle(i, 0);
                    MOTOR_TRACE("E Duty", result);
                    if (fabs(thisMotor->measuredSpeed) < 10) {
                        thisMotor->motorRunning = false;
                        //now stopped
                        if (--motorsRunning == 0) {
                            CancelCondition(MOTORS_BUSY);
                        }
                    }
                }
            } else {
                switch (i) {
                    case FRONT_LEFT_MOTOR:
                    case REAR_LEFT_MOTOR:
                        thisMotor->desiredSpeed = portMotors->desiredSpeed * decelerationFactor;
                        break;
                    case FRONT_RIGHT_MOTOR:
                    case REAR_RIGHT_MOTOR:
                        thisMotor->desiredSpeed = starboardMotors->desiredSpeed * decelerationFactor;
                        break;
                }
            }

            //calculate new duty cycle
            if (fabs(thisMotor->desiredSpeed) > 1) {
                if (!thisMotor->motorRunning) {
                        //starting
                        float result;
                        //need to start
                        if (motorsRunning++ == 0) {
                            SetCondition(MOTORS_BUSY);
                        }
                        //apply starting power
                        result = setDutyCycle(i, startingDuty);

                        MOTOR_TRACE("S Duty", result);
                        //               thisMotor->errorIntegral = 0;
                        thisMotor->motorRunning = true;

                    } else {
                        //running
                        float result;
                        float speedError;

                        if (motorSide[thisMotor->side].direction == MOTOR_FORWARD) {
                            speedError = (thisMotor->desiredSpeed - thisMotor->measuredSpeed);
                        } else {
                            speedError = -(thisMotor->desiredSpeed - thisMotor->measuredSpeed);
                        }
                        //need to change duty cycle
                        if (speedError > 0) {
                            result = setDutyCycle(i, thisMotor->currentDutyRatio + dutyIncrement);
                        } else {
                            result = setDutyCycle(i, thisMotor->currentDutyRatio - dutyIncrement);
                        }

                        MOTOR_TRACE("R Duty", result);
                    }
                }
            }

        xSemaphoreGive(motorStructMutex);

        if (motorsRunning) {
            DebugPrint("MOT: %.0f, %.0f to go", portMotors->distanceToGo, starboardMotors->distanceToGo);
        }

        if ((lastOdoReport + odoInterval < xTaskGetTickCount()) &&
                (portEncodersReported != portEncoders
                || starboardEncodersReported != starboardEncoders
                || motorsRunningReported != motorsRunning)) {

            //report odometry
            //movement since last message
            odoMsg.odometryPayload.portMovement = (float) MOVE_PER_COUNT * portEncoders / 2; //in cm;         // cm
            odoMsg.odometryPayload.starboardMovement = (float) MOVE_PER_COUNT * starboardEncoders / 2; //in cm;
            //current speed
            odoMsg.odometryPayload.portSpeed = (int16_t) portMotors->measuredSpeed; //cm per sec
            odoMsg.odometryPayload.starboardSpeed = (int16_t) starboardMotors->measuredSpeed; //cm per sec
            //current goal
            odoMsg.odometryPayload.portMotorsToGo = (int16_t) portMotors->distanceToGo; //cm to go
            odoMsg.odometryPayload.starboardMotorsToGo = (int16_t) starboardMotors->distanceToGo; //cm

            odoMsg.odometryPayload.motorsRunning = motorsRunning;

            psForwardMessage(&odoMsg, 50);

            portEncodersReported = portEncoders;
            starboardEncodersReported = starboardEncoders;
            motorsRunningReported = motorsRunning;

            lastOdoReport = xTaskGetTickCount();
        }
        vTaskDelayUntil(&LastWakeTime, (TickType_t) pidInterval);
    }
}

//load the Pwm register

float setDutyCycle(int motor, float _duty) {
    float duty;

    setDirection(motor, (motorSide[motors[motor].side].direction));
    
    duty = (_duty > maxDuty ? maxDuty : _duty);


    unsigned int regValue = (1 - fabs(duty)) * (timerPeriod);

    switch (motorToOutCompare[motor]) {
        case 2:
            SetDCOC2PWM(regValue);
            break;
        case 3:
            SetDCOC3PWM(regValue);
            break;
        case 4:
            SetDCOC4PWM(regValue);
            break;
        case 5:
            SetDCOC5PWM(regValue);
            break;
    }
    motors[motor].currentDutyRatio = duty;

    return duty;
}

//set the direction pin

void setDirection(int motor, MotorDirectionEnum_t direction) {
    IoPortId dirPortId;
    unsigned int dirBit;
    int newDirectionValue;
    //lookup direction pin address
    switch (motorToConnector[motor]) {
        case 1:
            dirPortId = DIR_1_IOPORT;
            dirBit = DIR_1_BIT;
            break;
        case 2:
            dirPortId = DIR_2_IOPORT;
            dirBit = DIR_2_BIT;
            break;
        case 3:
            dirPortId = DIR_3_IOPORT;
            dirBit = DIR_3_BIT;
            break;
        case 4:
            dirPortId = DIR_4_IOPORT;
            dirBit = DIR_4_BIT;
            break;
    }
    //adjust for motor position
    switch (motor) {
        case FRONT_LEFT_MOTOR:
        case REAR_LEFT_MOTOR:
            newDirectionValue = (direction == MOTOR_FORWARD ? 0 : 1);
            break;
        case FRONT_RIGHT_MOTOR:
        case REAR_RIGHT_MOTOR:
            newDirectionValue = (direction == MOTOR_FORWARD ? 1 : 0);
            break;
    }
    //set the direction pin
    if (newDirectionValue > 0) {
        PORTSetBits(dirPortId, dirBit);
    } else {
        PORTClearBits(dirPortId, dirBit);
    }
}
