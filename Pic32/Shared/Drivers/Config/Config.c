/*
 * File:   Config.c
 * Author: martin
 *
 * Created on April 18, 2014
 */
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "Messages.h"
#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"

#include "Config.h"

//Convenience functions to manage config

int configCount;             //count of config values sent

void SendBinaryOptionConfig(char *name, int *var, int req) {
    SendOptionConfig(name, var, 0, 1, req);
}

void SendOptionConfig(char *name, int *var, int minV, int maxV, int req) {
    psMessage_t msg;
    psInitPublish(msg, OPTION);
    strncpy(msg.optionPayload.name, name, PS_NAME_LENGTH);
    msg.optionPayload.value = *var;
    msg.optionPayload.min = minV;
    msg.optionPayload.max = maxV;
    msg.optionPayload.subsystem = (uint8_t) req;
    psSendMessage(msg);
    configCount++;
    vTaskDelay(MESSAGE_DELAY);
}

void SendSettingConfig(char *name, float *var, float minV, float maxV, int req) {
    psMessage_t msg;
    psInitPublish(msg, SETTING);
    strncpy(msg.settingPayload.name, name, PS_NAME_LENGTH);
    msg.settingPayload.value = *var;
    msg.settingPayload.min = minV;
    msg.settingPayload.max = maxV;
    msg.settingPayload.subsystem = (uint8_t) req;
    psSendMessage(msg);
    configCount++;
    vTaskDelay(MESSAGE_DELAY);
}

void SetOption(psMessage_t *msg, char *name, int *var, int minV, int maxV)
{
    if (strcmp(name, msg->optionPayload.name) == 0)
    {
        *var = msg->optionPayload.value;
        SendOptionConfig(name, var, minV, maxV, 0);
    }
}
void NewSetting(psMessage_t *msg, char *name, float *var, float minV, float maxV)
{
    if (strcmp(name, msg->settingPayload.name) == 0)
    {
        *var = msg->settingPayload.value;
        SendSettingConfig(name, var, minV, maxV, 0);
    }
}
void SendSetOption(char *name, int var, int minV, int maxV)
{
    psMessage_t msg;
    psInitPublish(msg, SET_OPTION);
    strncpy(msg.optionPayload.name, name, PS_NAME_LENGTH);
    msg.optionPayload.value = var;
    msg.optionPayload.min = minV;
    msg.optionPayload.max = maxV;
    msg.optionPayload.subsystem = 0;
    psSendMessage(msg);
}

void SetOptionByName(char *name, int value) {
    bool optionFound = false;
    
#define optionmacro(n, var, minV, maxV, def) if (strncmp(n,name,PS_NAME_LENGTH)==0) {\
                                                    var = value;\
                                                    optionFound = true;\
                                                    SendSetOption(name, var, minV, maxV);\
                                                    }
#include "Options.h"
#undef optionmacro
    
    if (!optionFound)
    {
        DebugPrint("SetOptionByName: %s not found\n", name);
    }
}

void SendIntValue(char *name, int value)
{
    psMessage_t msg;
    psInitPublish(msg, INT_DATA);
    strncpy(msg.nameIntPayload.name, name, PS_NAME_LENGTH);
    msg.nameIntPayload.value = value;
    psSendMessage(msg);
    vTaskDelay(100);
}

void SendFloatValue(char *name, int value)
{
    psMessage_t msg;
    psInitPublish(msg, FLOAT_DATA);
    strncpy(msg.nameFloatPayload.name, name, PS_NAME_LENGTH);
    msg.nameFloatPayload.value = value;
    psSendMessage(msg);
    vTaskDelay(100);
}

#define optionmacro(name, var, min, max, def) int var = def;
#include "Options.h"
#undef optionmacro

#define settingmacro(name,var,min,max,def) float var = def;
#include "Settings.h"
#undef settingmacro