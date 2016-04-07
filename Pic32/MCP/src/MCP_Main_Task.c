/*
 * File:   MCPtask.c
 * Author: martin
 *
 * Created on November 22, 2013
 */

//Responds to App messages and selected events

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define _SUPPRESS_PLIB_WARNING
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING
#include "plib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "Drivers/I2C/I2C.h"
#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSubData.h"
#include "PubSub/Notifications.h"
#include "PubSub/xbee.h"
#include "MCP.h"

#include "Drivers/Config/Config.h"

//RTOS task in this file
static void MCPtask(void *pvParameters);

//task input message queue
PubSubQueue_t mcpQueue;

int initReply = 0;
bool picUnitialized = true;
bool OVMonline = false;
bool MOTonline = false;
bool APPonline = false;

bool *onlineCondition[PS_UARTS_COUNT] = {&MOTonline, &OVMonline};

NotificationMask_t WakeEventMask = 0;

//Options and settings
void SetPinOption(unsigned int ioport, unsigned int iobit, int value);

void MCPtaskInit() {

    //start the MCP main task
    xTaskCreate(MCPtask, /* The function that implements the task. */
            (signed char *) "MCP Task", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
            MCP_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            MCP_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL); /* The task handle is not required, so NULL is passed. */
}

bool McpTaskProcessMessage(psMessage_t *msg, TickType_t wait) {

    //not filtered at present
    if (mcpQueue) {
        if (xQueueSendToBack(mcpQueue, msg, wait) != pdTRUE) {
            DebugPrint("MCP discarded %s", psLongMsgNames[msg->header.messageType]);
            return false;
        }
        else return true;
    }
    else return false;
}

psMessage_t mcpTaskRxMsg;   //static received message
psMessage_t mcpTxMsg; //static sent message

static void MCPtask(void *pvParameters) {
    int i;
    int initReply = 0;
    bool picUnitialized = true;

    //LED pin
    PORTSetPinsDigitalOut(USER_LED_IOPORT, USER_LED_BIT);
    PORTClearBits(USER_LED_IOPORT, USER_LED_BIT);

    //set default power state
    SetPinOption(MOT_PWR_IOPORT, MOT_PWR_BIT, motPower);
    SetPinOption(OVM_PWR_IOPORT, OVM_PWR_BIT, ovmPower);

    WakeEventMask = 0;

    initReply += SysLogInit();
    initReply += PubSubInit();
    initReply += NotificationsInit();

    CancelCondition(BATTERY_LOW);
    CancelCondition(BATTERY_CRITICAL);
    CancelCondition(CHARGING);
    CancelCondition(CHARGE_COMPLETE);
    CancelCondition(FRONT_CONTACT);
    CancelCondition(REAR_CONTACT);
    CancelCondition(FRONT_LEFT_PROXIMITY);
    CancelCondition(FRONT_CENTER_PROXIMITY);
    CancelCondition(FRONT_RIGHT_PROXIMITY);
    CancelCondition(REAR_LEFT_PROXIMITY);
    CancelCondition(REAR_CENTER_PROXIMITY);
    CancelCondition(REAR_RIGHT_PROXIMITY);
    CancelCondition(LEFT_PROXIMITY);
    CancelCondition(RIGHT_PROXIMITY);
    CancelCondition(FRONT_LEFT_FAR_PROXIMITY);
    CancelCondition(FRONT_RIGHT_FAR_PROXIMITY);
    CancelCondition(LEFT_PASSIVE_PROXIMITY);
    CancelCondition(RIGHT_PASSIVE_PROXIMITY);
    CancelCondition(RAINING);
    CancelCondition(SLEEPING);
    CancelCondition(MOT_POWERED);
    CancelCondition(OVM_POWERED);
    CancelCondition(SONAR_POWERED);
    CancelCondition(IR_POWERED);
    CancelCondition(PIR_ENABLED);
    CancelCondition(MCP_INIT_ERROR);
    CancelCondition(POWER_CONTROL_ERROR);
    CancelCondition(GPIO_ERROR);
    CancelCondition(I2C_BUS_ERROR);
    CancelCondition(MCP_ANALOG_ERROR);
    CancelCondition(RTC_ERROR);
    CancelCondition(PMIC_ERROR);
    CancelCondition(IR_PROXIMITY_ERROR);
    CancelCondition(SONAR_PROXIMITY_ERROR);
    CancelCondition(BUMPER_PROXIMITY_ERROR);
    CancelCondition(MCP_APP_COMMS_ERRORS);
    CancelCondition(MCP_APP_COMMS_CONGESTION);
    CancelCondition(MCP_MOT_COMMS_CONGESTION);
    CancelCondition(MCP_OVM_COMMS_CONGESTION);
    CancelCondition(MCP_MOT_COMMS_ERRORS);
    CancelCondition(MCP_OVM_COMMS_ERRORS);

    if ((mcpQueue = psNewPubSubQueue(MCP_TASK_QUEUE_LENGTH)) == NULL) {
        SetCondition(MCP_INIT_ERROR);
        LogError("No MCP Task Q");
        initReply += -1;
    }
    
    initReply += UARTBrokerInit();
    DebugPrint("UARTBrokerInit");
#ifdef PS_BLE_BROKER
    initReply += BLEBrokerInit();
    DebugPrint("BLEBrokerInit");
#endif
#ifdef XBEE_BROKER
    initReply += XBeeBrokerInit();
    DebugPrint("XBeeBrokerInit");
#endif

    initReply += PowerControlInit(POWER_SLEEPING);
    DebugPrint("PowerControlInit");
    initReply += ProximityInit();
    DebugPrint("ProximityInit");

    initReply += Analog_TaskInit();
    DebugPrint("Analog_TaskInit");
    initReply += I2C1_TaskInit();        //sonar
    DebugPrint("I2C1_TaskInit");
    initReply += Tick_TaskInit();
    DebugPrint("Tick_TaskInit");
    initReply += I2C4_TaskInit();      //GPIO, RTC
    DebugPrint("I2C4_TaskInit");

    psInitPublish(mcpTxMsg, SS_ONLINE);
    strcpy(mcpTxMsg.responsePayload.subsystem, "MCP");
    mcpTxMsg.responsePayload.flags = (initReply < 0 ? RESPONSE_INIT_ERRORS : 0)
            || RESPONSE_FIRST_TIME;
    psSendMessage(mcpTxMsg);

    if (initReply < 0) {
        SetCondition(MCP_INIT_ERROR);
        DebugPrint("MCP Init Error");
        
        while (1) {
            vTaskDelay(100);
            PORTToggleBits(USER_LED_IOPORT, USER_LED_BIT);
        }
    }
    
    LogRoutine("MCP Up");
    
    vTaskDelay(1000);
    
    ProcessUserCommand(COMMAND_ACTIVE);
    
    for (;;) {
        //Main Responder Loop
        //wait for a message
        if (xQueueReceive(mcpQueue, &mcpTaskRxMsg, portMAX_DELAY) == pdTRUE) {

            DebugPrint("MCP> message: %s", psLongMsgNames[mcpTaskRxMsg.header.messageType]);
    
            switch (mcpTaskRxMsg.header.messageType) {

                case CONFIG:
                    //send config vars
                    if (mcpTaskRxMsg.configPayload.responder == MCP_SUBSYSTEM) {
                         int requestor = mcpTaskRxMsg.configPayload.requestor;
                        configCount = 0;

#define optionmacro(name, var, minV, maxV, def) SendOptionConfig(name, &var, minV, maxV, requestor);
#include "Options.h"
#undef optionmacro

#define settingmacro(name, var, minV, maxV, def) SendSettingConfig(name, &var, minV, maxV, requestor);
#include "Settings.h"
#undef settingmacro

                        //report Config done
                        psInitPublish(mcpTxMsg, CONFIG_DONE);
                        mcpTxMsg.configPayload.count = configCount;
                        mcpTxMsg.configPayload.requestor = requestor;
                        mcpTxMsg.configPayload.responder = MCP_SUBSYSTEM;
                        psSendMessage(mcpTxMsg);

                        PublishConditions(true);
                        
                        LogRoutine("Config Sent");
                    }
                    break;
                case SET_OPTION:
                    LogRoutine("New Option %s = %i", mcpTaskRxMsg.optionPayload.name, mcpTaskRxMsg.optionPayload.value);
                    
#define optionmacro(n, var, minV, maxV, def) SetOption(&mcpTaskRxMsg, n, &var, minV, maxV);
#include "Options.h"
#undef optionmacro
       
                    if (strcmp(mcpTaskRxMsg.optionPayload.name, "Power Motors") == 0) SetPinOption(MOT_PWR_IOPORT, MOT_PWR_BIT, motPower);
                    if (strcmp(mcpTaskRxMsg.optionPayload.name, "Power Overmind") == 0) SetPinOption(OVM_PWR_IOPORT, OVM_PWR_BIT, ovmPower);
                    if (strcmp(mcpTaskRxMsg.optionPayload.name, "Solar Panel") == 0) SetPinOption(SOLAR_IOPORT, SOLAR_BIT, solarEnable);
                    break;
                case NEW_SETTING:
                    LogRoutine("New Setting %s = %f", mcpTaskRxMsg.settingPayload.name, mcpTaskRxMsg.settingPayload.value);
                    
#define settingmacro(n, var, minV, maxV, def) NewSetting(&mcpTaskRxMsg, n, &var, minV, maxV);
#include "Settings.h"
#undef settingmacro
                    
                    if (strncmp("XBEE Power Level",mcpTaskRxMsg.settingPayload.name,PS_NAME_LENGTH) == 0) SetPowerLevel((int) powerLevel);

                    break;
                case PING_MSG:
                {
                    LogRoutine("Ping Msg");
                    PORTToggleBits(USER_LED_IOPORT, USER_LED_BIT);

                    psInitPublish(mcpTxMsg, PING_RESPONSE)
                        strcpy(mcpTxMsg.responsePayload.subsystem, "MCP");
                    mcpTxMsg.responsePayload.flags = (initReply < 0 ? RESPONSE_INIT_ERRORS : 0)
                            || (picUnitialized ? RESPONSE_FIRST_TIME : 0);
                    mcpTxMsg.responsePayload.requestor = mcpTaskRxMsg.requestPayload.requestor;

                    psSendMessage(mcpTxMsg);
                    picUnitialized = 0;
                    
                    psInitPublish(mcpTxMsg, POWER_STATE);
                    mcpTxMsg.bytePayload.value = (uint8_t) systemPowerState;
                    psSendMessage(mcpTxMsg);                    
                    
                    PublishConditions(true);
                    
                    SendProximityReport(true);
                }
                    break;
                case COMMAND: //change commanded power state
                    ProcessUserCommand(mcpTaskRxMsg.bytePayload.value);
                    picUnitialized = 0;
                    break;
                    //Notifications that will wake the Overmind
                case WAKE_MASK:
                    WakeEventMask = mcpTaskRxMsg.eventMaskPayload.value;
                    break;
                    //Notifications raised elsewhere
                case NOTIFY:
                    switch (mcpTaskRxMsg.intPayload.value)
                    {
                        case BATTERY_SHUTDOWN_EVENT:
                            if (systemPowerState >= POWER_OVM_STARTING) {
                                //give the BBB a chance to stop
                                if (StartOvermindPoweroff(POWER_SHUTDOWN)) //start timer to kill the BBB if it doesn't power off
                                    LogInfo("Battery Shutdown");
                            } else {
                                SetPowerState(POWER_SHUTDOWN);
                            }          
                            break;
                        default:
                            if (WakeEventMask & NOTIFICATION_MASK(mcpTaskRxMsg.intPayload.value))
                            {
                                if (systemPowerState < POWER_OVM_STARTING) {
                                    if (StartOvermindPowerup(POWER_ACTIVE)) //start timer to kick the BBB if it doesn't power up
                                        LogInfo("Powerup timer -> Active");
                                }
                            }
                            break;
                    }
                    break;
                case OVM_COMMAND:
                    ProcessOvermindCommand(mcpTaskRxMsg.bytePayload.value);
                    break;
                 case GEN_STATS:
                     DebugPrint("Send Stats\n");
                    //RTOS task stats
                    GenerateRunTimeTaskStats();
                    GenerateRunTimeSystemStats();
                    break;
                default:
                    //ignore anything else
                    break;
            }
        }
    }
}

//Options and settings

void SetPinOption(unsigned int ioport, unsigned int iobit, int value) {
    if (value) {
        PORTSetBits(ioport, iobit);
    } else {
        PORTClearBits(ioport, iobit);
    }
}

