/*
 * File:   PowerControl.c
 * Author: martin
 *
 * Created on November 22, 2013
 */

//Responds needs to change power

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


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

#include "MCP.h"

//handle power state changes
PowerState_enum systemPowerState = POWER_SLEEPING; //current power state
UserCommand_enum AppPowerCommand = 0;
OvermindPowerCommand_enum OvermindPowerCommand = 0;

SemaphoreHandle_t powerStateMutex; //mutex for power state data

char *stateNames[] = POWER_STATE_NAMES; //for state reporting

//commanded power state from App
char *appCmdnames[] = USER_COMMAND_NAMES;

//commanded power state from BBB
char *ovmCmdNames[] = OVERMIND_COMMAND_NAMES;

//BBB Pwr Button
void pushPwrButton();

//powerup

//timer to deal with BBB fail to boot
TimerHandle_t overmindPowerupTimer;

PowerState_enum powerupTimerNextState = POWER_STATE_UNKNOWN; //non-zero when ticking

void powerupTimerCallback(TimerHandle_t xTimer);

//poweroff

//timer to deal with BBB fail to poweroff
TimerHandle_t overmindPoweroffTimer;

PowerState_enum poweroffTimerNextState = POWER_STATE_UNKNOWN; //non-zero when ticking

void poweroffTimerCallback(TimerHandle_t xTimer);

void picShutdown();

int PowerControlInit(PowerState_enum initialState) {
    powerStateMutex = xSemaphoreCreateMutex();

    //BBB Power button
    PORTSetPinsDigitalOut(BBB_BTN_IOPORT, BBB_BTN_BIT);
    PWR_BTN_OD(); //open drain
    PORTSetBits(BBB_BTN_IOPORT, BBB_BTN_BIT);
    PORTSetPinsDigitalIn(BBB_RST_IOPORT, BBB_RST_BIT);

    overmindPowerupTimer = xTimerCreate("Powerup", // Just a text name, not used by the kernel.
            POWERUP_TIMER_PERIOD, // The timer period in ticks.
            pdFALSE, // The timer will auto-reload itself when it expires.
            (void *) 0,
            powerupTimerCallback
            );
    if (overmindPowerupTimer == NULL) {
        LogError("power: No Powerup Timer");
        SetCondition(POWER_CONTROL_ERROR);
        return -1;
    }
    overmindPoweroffTimer = xTimerCreate("Poweroff", // Just a text name, not used by the kernel.
            POWEROFF_TIMER_PERIOD, // The timer period in ticks.
            pdFALSE, // The timer will auto-reload itself when it expires.
            (void *) 0,
            poweroffTimerCallback
            );
    if (overmindPoweroffTimer == NULL) {
        LogError("power: No Poweroff Timer");
        SetCondition(POWER_CONTROL_ERROR);
        return -1;
     }

    if (initialState > POWER_OVM_STARTING)
    {
        StartOvermindPowerup(initialState);
    }
    else
    {
        SetPowerState(initialState);
    }
    return 0;
}

//robot state needs to changed. Take action as needed
psMessage_t mcpTxMsg;
//1. New command from User via App message

void ProcessUserCommand(UserCommand_enum userCommand) {

    if (userCommand < 0 || userCommand >= COMMAND_COUNT) {
        LogError("power: Bad App Cmd: %i", userCommand);
        SetCondition(POWER_CONTROL_ERROR);
        return;
    }

    DebugPrint("power: App Cmd: %s", appCmdnames[userCommand]);

    switch (userCommand) {
        case COMMAND_SYSTEM_OFF: //to minimum power
            if (systemPowerState >= POWER_OVM_STARTING) {
                //give the BBB a chance to stop
                if (StartOvermindPoweroff(POWER_SHUTDOWN)) //start timer to kill the BBB if it doesn't power off
                    LogInfo("power: Poweroff - App Shutdown");
            } else {
                SetPowerState(POWER_SHUTDOWN);
            }
            break;
        case COMMAND_SLEEP: //only PIC + comms
            if (systemPowerState >= POWER_OVM_STARTING) {
                //give the BBB a chance to stop
                if (StartOvermindPoweroff(POWER_SLEEPING)) //start timer to kill the BBB if it doesn't power off
                    LogInfo("power: Poweroff - App Sleep");
            } else {
                SetPowerState(POWER_SLEEPING);
            }
            break;
        case COMMAND_REST: //OVM + minimum power
            if (systemPowerState < POWER_OVM_STARTING) {
                if (StartOvermindPowerup(POWER_RESTING)) //start timer to kick the BBB if it doesn't power up
                    LogInfo("power: Powerup timer -> Rest");
            }
            else {
                SetPowerState(POWER_RESTING);
            }
            break;
        case COMMAND_STANDBY: //ready to move
            if (systemPowerState < POWER_OVM_STARTING) {
                if (StartOvermindPowerup(POWER_STANDBY)) //start timer to kick the BBB if it doesn't power up
                    LogInfo("power: Powerup timer -> Stby");
            }
            else {
                SetPowerState(POWER_STANDBY);
            }
            break;
        case COMMAND_ACTIVE:
            if (systemPowerState < POWER_OVM_STARTING) {
                if (StartOvermindPowerup(POWER_ACTIVE)) //start timer to kick the BBB if it doesn't power up
                    LogInfo("power: Powerup timer -> Act");
            }
            else {
                SetPowerState(POWER_ACTIVE);
            }
            break;
    }
}

//2. New command from Overmind

void ProcessOvermindCommand(OvermindPowerCommand_enum overmindCommand) {

    if (overmindCommand < 0 || overmindCommand >= OVERMIND_ACTION_COUNT) {
        LogError("power: Bad Ovm Cmd: %i", overmindCommand);
        return;
    }

    DebugPrint("power: OVM Cmd: %s", ovmCmdNames[overmindCommand]);

    //check context
    switch (systemPowerState) {
        case POWER_OVM_STARTING:
            //OVM up, so cancel powerup timer
            xTimerStop(overmindPowerupTimer, portMAX_DELAY);
            powerupTimerNextState = 0;
            break;
        case POWER_OVM_STOPPING:
            //poweroff timer running, so ignore command
            return;
            break;
        case POWER_ACTIVE:
        case POWER_STANDBY: // == system STANDBY
        case POWER_RESTING: // BBB plus minimum power
            //command acceptable
            break;
        case POWER_WAKE_ON_EVENT: //wake if Prox or other criteria met
        case POWER_SLEEPING:    //minimum power state with comms
        case POWER_SHUTDOWN:    //eg low battery shutdown
            LogError("power: %s in MCP-only state", ovmCmdNames[overmindCommand]);
            return;
            break;
    }

    switch (overmindCommand) {
        default:
            break;
        case OVERMIND_POWEROFF: //poweroff totally
            if (StartOvermindPoweroff(POWER_SHUTDOWN)) //start timer to kill the BBB if it doesn't power off
                LogInfo("power: Poweroff - Shutdown");
            break;
        case OVERMIND_SLEEP: //keep MCP plus comms up
            if (StartOvermindPoweroff(POWER_SLEEPING)) //start timer to kill the BBB if it doesn't power off
                LogInfo("power: Poweroff - Sleeping");
            break;
        case OVERMIND_WAKE_ON_EVENT: //wake on bumper contact
            if (StartOvermindPoweroff(POWER_WAKE_ON_EVENT)) //start timer to kill the BBB if it doesn't power off
                LogInfo("power: Poweroff - Waiting");
            break;
        case OVERMIND_REQUEST_RESTING:
            switch (systemPowerState) {
                case POWER_ACTIVE:
                case POWER_STANDBY: // == system STANDBY
                    SetPowerState(POWER_RESTING);
                    break;
                default:
                    break;
            }
        case OVERMIND_REQUEST_ACTIVE:
            switch (systemPowerState) {
                case POWER_RESTING:
                case POWER_STANDBY: // == system STANDBY
                    SetPowerState(POWER_ACTIVE);
                    break;
                default:
                    break;
            }
            break;
    }
}

//get the power control pins to line up with the new power state

void SetPowerState(PowerState_enum state) {


    if (systemPowerState != state) {
        xSemaphoreTake(powerStateMutex, portMAX_DELAY);

        systemPowerState = state;

        //current power state
        bool MOTinhibit = true;
        bool shutdown = false;
        bool lightsOff = false;
        bool OVMon = ovmPower;
        bool MOTon = motPower;
        bool PROXon = irProxEnable;
        bool SONARon = sonarProxEnable;

        switch (state) {
            case POWER_OVM_STARTING:
                MOTinhibit = true;
                SONARon = false;
                OVMon = MOTon = PROXon = true;
                break;
            case POWER_OVM_STOPPING:
                //timers running
                break;
            case POWER_ACTIVE:
                MOTinhibit = false;
                SONARon = false;
                OVMon = MOTon = PROXon = true;
                break;
            case POWER_STANDBY: // == system STANDBY
                MOTinhibit = true;
                SONARon = false;
                OVMon = MOTon = PROXon = true;
                break;
            case POWER_RESTING: // BBB plus minimum power
                MOTinhibit = true;
                OVMon = true;
                SONARon = MOTon = PROXon = false;
                break;
            case POWER_WAKE_ON_EVENT: //wake if Prox or other criteria met
                if (WakeEventMask & NOTIFICATION_MASK(PROXIMITY_EVENT))
                {
                    PROXon = true;
                }
                else {
                    PROXon = false;
                }
                SONARon = MOTon = OVMon = false;
                MOTinhibit = true;
                lightsOff = true;
                break;
            case POWER_SLEEPING: //minimum power state with comms
                SONARon = OVMon = MOTon = PROXon = false;
                MOTinhibit = true;
                lightsOff = true;
                break;
            case POWER_SHUTDOWN: //eg low battery shutdown
                SONARon = OVMon = MOTon = PROXon = false;
                shutdown = true;
                MOTinhibit = true;
                lightsOff = true;
                break;
        }
        //set power lines appropriately
        if (lightsOff)
        {
            SetOptionByName("Lights Nav", 0);
            SetOptionByName("Lights Front",  0);
            SetOptionByName("Lights Rear",  0);
            SetOptionByName("Lights Strobe",  0);
        }
        
        SetOptionByName("Power IR Prox", PROXon);
        SetOptionByName("Power Sonar Prox", SONARon);
        SetOptionByName("Power Overmind", OVMon);
        SetOptionByName("Power Motors", MOTon);

        Condition(IR_POWERED, PROXon);
        Condition(SONAR_POWERED, SONARon);
        Condition(OVM_POWERED, OVMon);
        Condition(MOT_POWERED, MOTon);

        xSemaphoreGive(powerStateMutex);

        //report power state
        psInitPublish(mcpTxMsg, POWER_STATE);
        mcpTxMsg.bytePayload.value = state;
        psSendMessage(mcpTxMsg);

        LogInfo("power: Pwr State = %s", stateNames[state]);

        if (shutdown) {
            vTaskDelay(5000); //let messages propagate
            vTaskSuspendAll();
            taskDISABLE_INTERRUPTS();
            picShutdown();
        }
    }
}

bool isOvermindAwake() {
    return PORTReadBits(BBB_RST_IOPORT, BBB_RST_BIT);
}

bool StartOvermindPoweroff(PowerState_enum nextState) {

    if (xTimerIsTimerActive(overmindPowerupTimer)) {
        //kill powerup timer
        xTimerStop(overmindPowerupTimer, portMAX_DELAY);
        powerupTimerNextState = 0;
    }

    if (!xTimerIsTimerActive(overmindPoweroffTimer)) {

        NotifyEvent(SLEEPING_EVENT);

        poweroffTimerNextState = nextState;
        xTimerStart(overmindPoweroffTimer, portMAX_DELAY);
        
        if (bbbPwrBtnStop) {
            LogInfo("power: PwrBtn for poweroff");
            pushPwrButton();
        }
        SetPowerState(POWER_OVM_STOPPING);
        return true;
    } else {
        poweroffTimerNextState = nextState; //update destination state
    }
    return false;
}

int powerupCycleCount;

bool StartOvermindPowerup(PowerState_enum nextState) {
    //kill poweroff timer
    if (xTimerIsTimerActive(overmindPoweroffTimer)) {
        xTimerStop(overmindPoweroffTimer, portMAX_DELAY);
        poweroffTimerNextState = 0;
    }

    if (!xTimerIsTimerActive(overmindPowerupTimer)) {
        powerupCycleCount = 0;
        xTimerStart(overmindPowerupTimer, portMAX_DELAY);
        powerupTimerNextState = nextState;
        if (bbbPwrBtnStart && !isOvermindAwake()) {
            LogInfo("power: PwrBtn for powerup");
            pushPwrButton();
        }
        SetPowerState(POWER_OVM_STARTING);
        return true;
    } else {
        powerupTimerNextState = nextState; //update destination state
    }
    return false;
}

void pushPwrButton() {
    vTaskDelay(pwrBtnDelay * 1000);
    PORTClearBits(BBB_BTN_IOPORT, BBB_BTN_BIT);
    vTaskDelay(pwrBtnLength * 1000);
    PORTSetBits(BBB_BTN_IOPORT, BBB_BTN_BIT);
}

void picShutdown() {
    OSCCONSET = 0x10; //SLPEN Sleep Enable
    while (1)
        asm volatile("wait"); //sleep
}

void powerupTimerCallback(TimerHandle_t xTimer) {
    LogInfo("power: Powerup timer fired");
    if (isOvermindAwake()) {
        SetPowerState(powerupTimerNextState);
        powerupTimerNextState = 0;
    } else {
        LogError("power: Overmind failed to start");
        //retry
        if (powerupCycleCount++ < POWERUP_RETRIES) {
            LogInfo("power: PwrBtn powerup retry");
            pushPwrButton();
            xTimerStart(overmindPowerupTimer, 0);
        } else {
            SetPowerState(POWER_SLEEPING);
        }
    }
}

void poweroffTimerCallback(TimerHandle_t xTimer) {
    LogInfo("power: Power off timer fired");
    SetPowerState(poweroffTimerNextState);
    poweroffTimerNextState = 0;
}