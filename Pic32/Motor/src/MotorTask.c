/*
 * File:   MotorTask.c
 * Author: martin
 *
 * Created on January 25, 2014
 */

//Task to handle messages from other processors

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define _SUPPRESS_PLIB_WARNING
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING

#include "plib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSubData.h"

#include "Drivers/Config/Config.h"

#include "Motor.h"

bool MCPonline = false;

bool *onlineCondition[PS_UARTS_COUNT] = {&MCPonline};

static void MOTtask(void *pvParameters);

PubSubQueue_t motQueue;

PowerState_enum powerState = POWER_STATE_UNKNOWN;

bool MotTaskProcessMessage(psMessage_t *msg, TickType_t wait)
{
    if (motQueue)
    {
        if (xQueueSendToBack(motQueue, msg, wait) != pdTRUE) {
            DebugPrint("MOT discarded %s", psLongMsgNames[msg->header.messageType]);
            return false;
        }
        return true;
    }
    return false;
}

void MOTtaskInit() {

    //make sure the motors are disabled
    PORTSetPinsDigitalOut(MOT_RST_IOPORT, MOT_RST_BIT);
    PORTSetBits(MOT_RST_IOPORT, MOT_RST_BIT);
    //set all PWM ports high
    PORTSetPinsDigitalOut(PWM_1_IOPORT, PWM_1_BIT);
    PORTSetBits(PWM_1_IOPORT, PWM_1_BIT);
    PORTSetPinsDigitalOut(PWM_2_IOPORT, PWM_2_BIT);
    PORTSetBits(PWM_2_IOPORT, PWM_2_BIT);
    PORTSetPinsDigitalOut(PWM_3_IOPORT, PWM_3_BIT);
    PORTSetBits(PWM_2_IOPORT, PWM_3_BIT);
    PORTSetPinsDigitalOut(PWM_4_IOPORT, PWM_4_BIT);
    PORTSetBits(PWM_2_IOPORT, PWM_4_BIT);

    //start the MOT main task
    if (xTaskCreate(MOTtask, /* The function that implements the task. */
            (signed char *) "Motor Task", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
            MOT_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            MOT_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL) /* The task handle is not required, so NULL is passed. */
            != pdPASS) {
        DebugPrint( "MOT Task Fail");
        while(1);
    }
}

static psMessage_t motRxMsg;

static void MOTtask(void *pvParameters) {
    int commsStatsCount = commStatsSecs;
    int initReply = 0;
    bool initialized = false;
    
    //start subsystem tasks
    initReply += SysLogInit();
    initReply += PubSubInit();
    initReply += NotificationsInit();
    initReply += UARTBrokerInit();
    initReply += PidInit();      //Controls the motor PWM
    initReply += EncoderInit();  //Measures motor speed
    //initReply += AmpsInit();   //Measures motor current

    while ((motQueue = psNewPubSubQueue(MOT_TASK_QUEUE_LENGTH)) == NULL) {
        SetCondition(MOT_INIT_ERROR);
        LogError( "Motor Task Q");
        initReply = -1;
    }
    
    if (initReply < 0) {
        SetCondition(MOT_INIT_ERROR);
        DebugPrint("MOT Init Error");
        
        while (1) {
            vTaskDelay(100);
            PORTToggleBits(USER_LED_IOPORT, USER_LED_BIT);
        }
    }
    
    CancelCondition(MOT_INIT_ERROR);
    CancelCondition(MOTORS_INHIBIT);
    CancelCondition(MOTORS_BUSY);
    CancelCondition(MOT_ANALOG_ERROR);
    CancelCondition(MOTORS_ERRORS);
    CancelCondition(MOT_MCP_COMMS_ERRORS);
    CancelCondition(MOT_MCP_COMMS_CONGESTION);

    {
        psMessage_t msg;
        psInitPublish(msg, SS_ONLINE);
        strcpy(msg.responsePayload.subsystem, "MOT");
        msg.responsePayload.flags = RESPONSE_FIRST_TIME;
        psSendMessage(msg);
    }
    
    DebugPrint("MOT Up");

    for (;;) {
        

        //wait for a message
        if (xQueueReceive(motQueue, &motRxMsg, portMAX_DELAY) == pdTRUE) {

            switch (motRxMsg.header.messageType) {
                case TICK_1S:
                    powerState = motRxMsg.tickPayload.systemPowerState;

                    if (--commsStatsCount <= 0) {
                        commsStatsCount = commStatsSecs;
                        if (commStats) {
                            UARTSendStats();
                        } else {
                            UARTResetStats();
                        }
                    }
                    break;
                case CONFIG:
                    if (motRxMsg.configPayload.responder == MOTOR_SUBSYSTEM) {
                        DebugPrint("Config message received");
                        int requestor = motRxMsg.configPayload.requestor;

                       configCount = 0;
#define optionmacro(name, var, minV, maxV, def) SendOptionConfig(name, &var, minV, maxV, requestor);
#include "Options.h"
#undef optionmacro

#define settingmacro(name, var, minV, maxV, def) SendSettingConfig(name, &var, minV, maxV, requestor);
#include "Settings.h"
#undef settingmacro
                        //report Config done
                        psInitPublish(motRxMsg, CONFIG_DONE);
                        motRxMsg.configPayload.requestor = requestor;
                        motRxMsg.configPayload.responder = MOTOR_SUBSYSTEM;
                        motRxMsg.configPayload.count = configCount;
                        psSendMessage(motRxMsg);

                        DebugPrint("Config sent");
                    }
                    break;
                case SET_OPTION: //option change
                    DebugPrint("New Option %s = %i", motRxMsg.optionPayload.name, motRxMsg.optionPayload.value);

#define optionmacro(n, var, minV, maxV, def) SetOption(&motRxMsg, n, &var, minV, maxV);
#include "Options.h"
#undef optionmacro
                    break;
                case NEW_SETTING: //setting change
                    DebugPrint("New Setting %s = %f", motRxMsg.settingPayload.name, motRxMsg.settingPayload.value);

#define settingmacro(n, var, minV, maxV, def) NewSetting(&motRxMsg, n, &var, minV, maxV);
#include "Settings.h"
#undef settingmacro
                    break;
                case PING_MSG:
                {
                    psMessage_t msg2;

                    DebugPrint("Ping Msg");

//                    PORTToggleBits(USER_LED_IOPORT, USER_LED_BIT);

                    psInitPublish(msg2, PING_RESPONSE)
                            
                    strncpy(msg2.responsePayload.subsystem, "MOT", 3);
                    msg2.responsePayload.flags = (initialized ? 0 : RESPONSE_FIRST_TIME);  
                    msg2.responsePayload.requestor = motRxMsg.requestPayload.requestor;
                  
                    psSendMessage(msg2);
                }

                    break;
                case GEN_STATS:
                    DebugPrint("Send Stats\n");
                    GenerateRunTimeTaskStats();
                    GenerateRunTimeSystemStats();
                    break;
                default:
                    break;
            }
        }
    }
}
