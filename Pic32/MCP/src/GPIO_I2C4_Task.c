/*
 * File:   I2C4_Task.c
 * Author: martin
 *
 */

//looks after GPIO, lights and PIR sensors

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

#include "MCP.h"

//last driver result
I2C_StateMachine_enum I2C4_Result;

bool PMIC_errorLogged = false;
bool GPIO_errorLogged = false;
bool GPIO_powered = false;

//PMIC
PMICstate_enum PMICstate = PMIC_UNKNOWN;
PMICcommand_enum NewPMICcommand = 0;

//PIR
int PIRoperational = 0;
bool PIRnotified = false;

//lights
static uint8_t lightsCurrent = 0;

//task fucntion
static void I2C4_Task(void *pvParameters);

//AUX Functions
int GPIOinit();
int GPIOwrite(uint8_t _bits);
int GPIOread();
int GPIOdone();

//PMIC access
int PMICRead();
int PMICWrite(PMICcommand_enum cmd);

#define LIGHT_CYCLE 6
#define REPEAT_COUNT 10
#define CYCLE_DELAY 200

#define TASK_LOOP_MS 200

static uint8_t lightMasks[LIGHT_CYCLE] = {
    LEFT_RED_LED,
    FRONT_LEFT_LED,
    FRONT_RIGHT_LED,
    RIGHT_GREEN_LED,
    REAR_RIGHT_LED,
    REAR_LEFT_LED,
};

int I2C4_TaskInit() {
    //start the task
    if (xTaskCreate(I2C4_Task, /* The function that implements the task. */
            (signed char *) "GPIO_I2C", /* The text name assigned to the task*/
            I2C4_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            I2C4_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL) /* The task handle is not required, so NULL is passed. */
            != pdPASS) {
        LogError("I2C4 Task");
        SetCondition(MCP_INIT_ERROR);
        return -1;
    }
    return 0;
}

static void I2C4_Task(void *pvParameters) {
    TickType_t xLastWakeTime;
    int strobeLoopCount = 10;
    uint8_t strobeMask = 0;
    
    if (!I2C_Begin(GPIO_I2C)) {
        LogError("I2C4: I2C_Begin");
        SetCondition(I2C_BUS_ERROR);
        SetCondition(GPIO_ERROR);
        vTaskSuspend(NULL);
    }

    //initialize MCP23008 GPIO
    while (GPIOinit() < 0) {
        //cycle power
        GPIOdone();
        vTaskDelay(10000);
    }
    {
        int i, j, res = 0;
        for (i = 0; i < REPEAT_COUNT; i++) {
            for (j = 0; j < LIGHT_CYCLE; j++) {
                res = GPIOwrite(lightMasks[j]);
                if (res < 0) break;
                vTaskDelay(CYCLE_DELAY);
            }
            if (res < 0) break; //give up on error
        }
        GPIOwrite(0);
    }

    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        
        if (PIRoperational || navLights || frontLights || rearLights || strobeLights) {

            if (!GPIO_powered) {
                //need aux power and GPIO configured
                while (GPIOinit() < 0) {
                    //cycle power
                    GPIOdone();
                    vTaskDelay(10000);
                }
            }
            //check if PIR to be read
            if (PIRoperational > 1) {
                //need to wait
                PIRoperational--;
                if (PIRoperational == 1)
                {
                   LogInfo("PIR online\n");
                }
            } else if (PIRoperational == 1) {
                int GPIO = GPIOread();

                if (GPIO >= 0) {
                    ProximityReview(LEFT_PASSIVE_IR, ((~GPIO) & LEFT_PIR));
                    ProximityReview(RIGHT_PASSIVE_IR, ((~GPIO) & RIGHT_PIR));
                } else {
                    //read error
                    ProximityReview(LEFT_PASSIVE_IR, false);
                    ProximityReview(RIGHT_PASSIVE_IR, false);
                }
            } else {
                //offline
                ProximityReview(LEFT_PASSIVE_IR, false);
                ProximityReview(RIGHT_PASSIVE_IR, false);
            }

            //TODO: check humidity & temperature

            //update lights if needed
            uint8_t lights = 0;
            if (navLights) lights |= NAV_LIGHTS;
            if (frontLights) lights |= FRONT_LIGHTS;
            if (rearLights) lights |= REAR_LIGHTS;

            if (strobeLights) {
                if (--strobeLoopCount < 0) {
                    strobeMask ^= (FRONT_LIGHTS | REAR_LIGHTS);
                    strobeLoopCount = (int) strobeInterval / TASK_LOOP_MS;
                }
            }
            else strobeMask = 0;
            
            lights ^= strobeMask;
            
            if (lights != lightsCurrent) {
                if (GPIOwrite(lights) == 0)
                    lightsCurrent = lights;
            }
        } else {
            GPIOdone();
        }
        
        //activate Passive IR
        if (pirProxEnable)
        {
            if (PIRoperational == 0)
            {
                PIRoperational = PIR_START_WAIT * 1000 / TASK_LOOP_MS;  //30 seconds
                LogInfo("i2c4: Start PIR Wait]n");
            }
        }
        else
        {
            PIRoperational = 0;
        }
        
        
        //check PMIC status
//        PMICRead();

        //send command if needed
//        if (NewPMICcommand != 0) {
//            PMICWrite(NewPMICcommand);
//            NewPMICcommand = 0;
//        }

        vTaskDelayUntil(&xLastWakeTime, TASK_LOOP_MS);
    }
}

int GPIOinit() {

    PORTSetBits(AUX_PWR_IOPORT, AUX_PWR_BIT);
    vTaskDelay(250); //wait for power to stabilize
    I2C4_Result = I2C_WriteRegister(GPIO_I2C, GPIO_I2C_ADDRESS, GPIO_IODIR, LEFT_PIR | RIGHT_PIR);
    if (I2C4_Result != I2C_OK) {
        if (!GPIO_errorLogged) {
            DebugPrint("i2c4: GPIO IODIR error %s", I2C_errorMsg(I2C4_Result));
            SetCondition(I2C_BUS_ERROR);
            SetCondition(GPIO_ERROR);
            GPIO_errorLogged = true;
            return -1;
        } else {
            GPIO_errorLogged = false;
        }
    }
    GPIO_powered = true;
    return 0;
}

int GPIOwrite(uint8_t _bits) {
    I2C4_Result = I2C_WriteRegister(GPIO_I2C, GPIO_I2C_ADDRESS, GPIO_PORT, _bits);
    if (I2C4_Result != I2C_OK) {
        if (!GPIO_errorLogged) {
            DebugPrint("i2c4: GPIO PORT error %s", I2C_errorMsg(I2C4_Result));
            SetCondition(I2C_BUS_ERROR);
            SetCondition(GPIO_ERROR);
            GPIO_errorLogged = true;
        } 
        GPIOdone();
        return -1;
    }
    GPIO_errorLogged = false;
    return 0;
}

int GPIOread() {
    uint8_t data;
    I2C4_Result = I2C_ReadRegister(GPIO_I2C, GPIO_I2C_ADDRESS, GPIO_PORT, &data);
    if (I2C4_Result != I2C_OK) {
        if (!GPIO_errorLogged) {
            DebugPrint("i2c4: GPIO PORT error %s", I2C_errorMsg(I2C4_Result));
            SetCondition(I2C_BUS_ERROR);
            SetCondition(GPIO_ERROR);
            GPIO_errorLogged = true;
        } 
        GPIOdone();
        return -1;
    }
    GPIO_errorLogged = false;
    return data;
}

int GPIOdone() {
    PORTClearBits(AUX_PWR_IOPORT, AUX_PWR_BIT);
    GPIO_powered = false;
    return 0;
}

PMICstate_enum getPMICstate() {
    return PMICstate;
}

void PMIC_Command(PMICcommand_enum _PMICcommand) {
    NewPMICcommand = _PMICcommand;
}

int PMICRead() {
    uint8_t readData, writeData;

    I2C4_Result = I2C_Read(GPIO_I2C, PMIC_I2C_ADDRESS, &writeData, 0, &readData, 1);
    if (I2C4_Result == I2C_OK) {
        PMIC_errorLogged = false;
        return readData;
    } else {
        if (!PMIC_errorLogged) {
            DebugPrint("PMIC R: %s", I2C_errorMsg(I2C4_Result));
            PMIC_errorLogged = true;
            SetCondition(I2C_BUS_ERROR);
            SetCondition(PMIC_ERROR);
        }
        return -1;
    }
}

int PMICWrite(PMICcommand_enum cmd) {
    uint8_t writeData = cmd;

    I2C4_Result = I2C_Write(GPIO_I2C, PMIC_I2C_ADDRESS, &writeData, 1);
    if (I2C4_Result != I2C_OK) {
        if (!PMIC_errorLogged) {
            SetCondition(PMIC_ERROR);
            DebugPrint("i2c4: PMIC W: %s", I2C_errorMsg(I2C4_Result));
            PMIC_errorLogged = true;
            SetCondition(I2C_BUS_ERROR);
            SetCondition(PMIC_ERROR);
        }
    } else {
        PMIC_errorLogged = false;
    }
    return I2C4_Result;
}