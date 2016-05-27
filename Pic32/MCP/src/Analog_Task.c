/*
 * File:   Analog.c
 * Author: martin
 *
 * Created on May 13, 2014
 */

//Makes analog measurements
//Analog connections include:
//  16:1 Prox mux output
//Increments Mux selector after each sample

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
#include "semphr.h"
#include "timers.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSub/Notifications.h"
#include "PubSubData.h"

#include "MCP.h"

//functions in this file
//the RTOS task
static void AnalogTask(void *pvParameters);

//set the analog mux
void SetMux(int channel);

//evaluate a measurement
void Evaluate(int source, float voltage);

//semaphore for ADC ISR sync
SemaphoreHandle_t doneSemaphore = NULL;

bool IRProxPowered; //Whether IR Proximity Sensors powered

TickType_t environmentNextReportTime; //next report on volts/amps etc.

//ADC 16 channel multiplexor control
volatile int mux; //current channel

//raw ADC samples loaded by ISR
volatile unsigned int pmuxSample[MUX_COUNT];
volatile unsigned int amuxSample[MUX_COUNT];

//count for sample averaging
#define MULTISAMPLE 1

//use of the mux channels
int pmuxChannels[] = PMUX_CHANNELS; //enum list in HardwareProfile
int amuxChannels[] = AMUX_CHANNELS;

char *amuxSourceNames[] = AMUX_SOURCE_NAMES; //volts and amps names

float analogSourceVoltages[TOTAL_SOURCE_COUNT]; //raw volts reading

float initialSourceVoltages[TOTAL_SOURCE_COUNT]; //initial raw volts reading

bool lastRainResult;
TickType_t rainResultChangeTime;
bool rainNotified;

bool lastMotorInhResult;
TickType_t motorInhResultChangeTime;
bool motorInhNotified;

//three rear digital ir sensors are wired together with voltage drop resistors
//these thresholds allow the three detectors to be separated
float comboThresholds[8] = {3.0,
    2.6,
    2.2,
    1.9,
    1.6,
    1.2,
    0.8,
    0};
//defines for the index into the threshold list
#define DIGITAL_PROX_LEFT 0x4
#define DIGITAL_PROX_CENTER 0x1
#define DIGITAL_PROX_RIGHT 0x2

//assumed 'virtual' prox device for each result
int proxSource[8] = {0, RC_IR_PROX, RR_IR_PROX, RR_IR_PROX, RL_IR_PROX,
    RL_IR_PROX, RC_IR_PROX, RC_IR_PROX};

//battery status for condition reporting
BatteryStatus_enum batteryStatus = 0;
 
//static message buffer
psMessage_t RawMsg;

int Analog_TaskInit() {

    //start the task
    if (xTaskCreate(AnalogTask, /* The function that implements the task. */
            (signed char *) "Analog", /* The text name assigned to the task*/
            ANALOG_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            ANALOG_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL) /* The task handle is not required, so NULL is passed. */
            != pdPASS) {
        LogError("Analog Task");
        SetCondition(MCP_ANALOG_ERROR);
        SetCondition(MCP_INIT_ERROR);
        return -1;
    }
    return 0;
}

static void AnalogTask(void *pvParameters) {
    int i;
    TickType_t xLastWakeTime;

    //create the semaphore to sync with the ISR
    doneSemaphore = xSemaphoreCreateBinary();

    if (doneSemaphore == NULL) {
        LogError("Analog Semphr");
        SetCondition(MCP_ANALOG_ERROR);
        vTaskSuspend(NULL); //outa here
    }

    xSemaphoreTake(doneSemaphore, 0); //make sure it's empty

    //initialize prox status
    for (i = 0; i < PROX_COUNT; i++) {
        switch (proxDeviceType[i]) {
            case VIRTUAL_FSR:
            case BUMPER_FSR:
                proxSensorData[i].status = PROX_ACTIVE;
                break;
            case ANALOG_IR:
            case DIGITAL_IR:
                proxSensorData[i].status = PROX_OFFLINE;
                break;
            default:
                break;
        }
    }

    //set the mux select pins to output
    //there are two multiplexors with separate select pins (to simplify the board)
    //one mainly for proximity
    PORTSetPinsDigitalOut(PMUX0_IOPORT, PMUX0_BIT);
    PORTSetPinsDigitalOut(PMUX1_IOPORT, PMUX1_BIT);
    PORTSetPinsDigitalOut(PMUX2_IOPORT, PMUX2_BIT);
    PORTSetPinsDigitalOut(PMUX3_IOPORT, PMUX3_BIT);
    //one for general analog stuff
    PORTSetPinsDigitalOut(AMUX0_IOPORT, PMUX0_BIT);
    PORTSetPinsDigitalOut(AMUX1_IOPORT, PMUX1_BIT);
    PORTSetPinsDigitalOut(AMUX2_IOPORT, PMUX2_BIT);
    PORTSetPinsDigitalOut(AMUX3_IOPORT, PMUX3_BIT);

    IRProxPowered = false;

    environmentNextReportTime = xTaskGetTickCount() + envReport;

    //select first channel
    mux = 0;
    SetMux(mux);

    //configure the ADC - MUXA AN9 alternate MUXB AN4

    INTEnable(INT_AD1, INT_DISABLED); //clear int enable
    AD1CON1bits.ON = 0; //ADC OFF

    AD1PCFG = ~(BIT_9 | BIT_4); //AN6 & AN14 Analog
    TRISBSET = (BIT_9 | BIT_4); //set for input

    AD1CON1bits.SIDL = 1; //Stop in Idle
    AD1CON1bits.FORM = 0; //16-bit integer
    AD1CON1bits.SSRC = 0b010; //Trigger = Timer3
    AD1CON1bits.CLRASAM = 0; //Do not stop after interrupt
    AD1CON1bits.ASAM = 1; //sample auto-start
    AD1CON2bits.VCFG = 0; //Vdd<>Vss reference
    AD1CON2bits.OFFCAL = 0; //offset cal off
    AD1CON2bits.CSCNA = 0; //No input scan
    AD1CON2bits.SMPI = (MULTISAMPLE * 2) - 1; //(2 * MULTISAMPLE) conversions per interrupt
    AD1CON2bits.BUFM = 0; //1 16-word buffer
    AD1CON2bits.ALTS = 1; //Alternate MuxA/B sampling, MuxA first
    AD1CON3bits.ADCS = 19; //Tad = Tpb x 40 = 1uS
    AD1CON3bits.ADRC = 0; //AD clock from PB clock
    AD1CHSbits.CH0SA = 9; //AN9 on Mux A
    AD1CHSbits.CH0NA = 0; //Vref-
    AD1CHSbits.CH0SB = 4; //AN4 on Mux B
    AD1CHSbits.CH0NB = 0; //Vref-
    AD1CSSL = 0; //no scanning

    //prepare timer for 1ms ticks - ends sampling, starts conversion. 32ms complete loop!
#define T3_COUNT (GetPeripheralClock() / 256000)
    OpenTimer3(T3_ON | T3_IDLE_STOP | T3_GATE_OFF | T3_PS_1_64 | T3_SOURCE_INT, T3_COUNT);

    AD1CON1bits.ON = 1; //ADC ON

    //config the interrupt
    ConfigIntADC10(ADC_INT_PRIORITY | ADC_INT_SUB_PRIORITY | ADC_INT_ON);

    AD1CON1bits.SAMP = 1; //start Sampling

    DebugPrint("analog: Task Up");

    //get initial voltages
    vTaskDelay(100);
    //clear semaphore
    xSemaphoreTake(doneSemaphore, 0);

    //wait for end of analog batch to avoid data corruption
    xSemaphoreTake(doneSemaphore, portMAX_DELAY);

    //save all mux channel voltages
    for (i = 0; i < MUX_COUNT; i++) {
        initialSourceVoltages[pmuxChannels[i]] = (float) (pmuxSample[i] / 1024.0f) * 3.3; //scale by ADC
        initialSourceVoltages[amuxChannels[i]] = (float) (amuxSample[i] / 1024.0f) * 3.3; //scale by ADC
        pmuxSample[i] = amuxSample[i] = 0;
    }

    //Analog task loop
    xLastWakeTime = xTaskGetTickCount(); //used for TaskDelayUntil

    for (;;) {

        //adjust IR sensor power as needed
        if (irProxEnable) { //option
            if (!IRProxPowered) {
                PORTSetBits(IR_PROX_PWR_IOPORT, IR_PROX_PWR_BIT);
                vTaskDelay(200);
                IRProxPowered = true;
                for (i = 0; i < PROX_COUNT; i++) {
                    switch (proxDeviceType[i]) {
                        case ANALOG_IR:
                        case DIGITAL_IR:
                            proxSensorData[i].status = PROX_ACTIVE;
                            break;
                        default:
                            break;
                    }
                }
            }
        } else {
            if (IRProxPowered) {
                PORTClearBits(IR_PROX_PWR_IOPORT, IR_PROX_PWR_BIT);
                IRProxPowered = false;
                for (i = 0; i < PROX_COUNT; i++) {
                    switch (proxDeviceType[i]) {
                        case ANALOG_IR:
                        case DIGITAL_IR:
                            proxSensorData[i].status = PROX_OFFLINE;
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        //clear semaphore
        xSemaphoreTake(doneSemaphore, 0);

        //wait for end of analog batch to avoid data corruption
        xSemaphoreTake(doneSemaphore, portMAX_DELAY);

        //save all mux channel voltages
        for (i = 0; i < MUX_COUNT; i++) {
            //scale from 0-1023 to 0-3.3v
            analogSourceVoltages[pmuxChannels[i]] = (float) (pmuxSample[i] * 3.3f) / 1024.0f; //scale by ADC
            analogSourceVoltages[amuxChannels[i]] = (float) (amuxSample[i] * 3.3f) / 1024.0f; //scale by ADC
     
//            if (pmuxChannels[i] == FC_IR_PROX) LogInfo("FCIR_V = 0x%03x, %.3f", pmuxSample[i], analogSourceVoltages[pmuxChannels[i]]);

        }

        //evaluate all mux channels
        for (i = 0; i < TOTAL_SOURCE_COUNT; i++) {
            Evaluate(i, analogSourceVoltages[i]);
        }

        //        DebugPrint("Prox Distant = 0x%2x, Close = 0x%2x", proxDistant, proxClose);

        //environment reporting
        if (environmentNextReportTime < xTaskGetTickCount()) {

            //generate volts etc. reports

            psInitPublish(RawMsg, ENVIRONMENT);
            RawMsg.environmentPayload.batteryVolts = (uint16_t) batteryVolts * 10; //times 10
            RawMsg.environmentPayload.batteryAmps   = (int16_t) batteryAmps * 10; //times 10
            RawMsg.environmentPayload.maxAmps       = (int16_t) maxAmps * 10;   //times 10
            RawMsg.environmentPayload.batteryAh     = (int16_t) batteryAh * 10; //times 10
            RawMsg.environmentPayload.solarVolts    = (uint16_t) solarVolts * 10; //times 10
            RawMsg.environmentPayload.chargeVolts   = (uint16_t) chargeVolts * 10; //times 10
            RawMsg.environmentPayload.solarAmps     = (uint8_t) solarAmps * 10; //times 10
            RawMsg.environmentPayload.chargeAmps    = (uint8_t) chargeAmps * 10; //times 10
            RawMsg.environmentPayload.internalTemp  = (uint8_t) int_temp;
            RawMsg.environmentPayload.externalTemp  = (uint8_t) ext_temp;
            RawMsg.environmentPayload.relativeHumidity = (uint8_t) humidity;
            RawMsg.environmentPayload.ambientLight  = (uint8_t) ambientLight;
            RawMsg.environmentPayload.isRaining     = (uint8_t) isRaining; //boolean
            psSendMessage(RawMsg);

            //additional reports

            //battery state check
            float motorAdj = (isConditionActive(MOTORS_BUSY) ? motorOffsetV : 0.0);
            
            if (batteryVolts < (shutdownV - motorAdj) || batteryAh < -shutdownAh ) {
                if (batteryStatus != BATTERY_SHUTDOWN_STATUS)
                {
                    NotifyEvent(BATTERY_SHUTDOWN_EVENT);
                    batteryStatus = BATTERY_SHUTDOWN_STATUS;
                }
            } else if (batteryVolts < (battCriticalV - motorAdj) || batteryAh < -battCriticalAh ) {
                if (batteryStatus != BATTERY_CRITICAL_STATUS)
                {
                    NotifyEvent(BATTERY_CRITICAL_EVENT);
                    batteryStatus = BATTERY_CRITICAL_STATUS;
                }
            } else if (batteryVolts < (battLowV - motorAdj) || batteryAh < -battLowAh ) {
                 if (batteryStatus != BATTERY_LOW_STATUS)
                {
                    NotifyEvent(BATTERY_LOW_EVENT);
                    batteryStatus = BATTERY_LOW_STATUS;
                }
            } else {
                batteryStatus = BATTERY_NOMINAL_STATUS;
            }

            Condition(BATTERY_CRITICAL, (batteryStatus >= BATTERY_CRITICAL_STATUS));
            Condition(BATTERY_LOW, (batteryStatus >= BATTERY_LOW_STATUS));
            
            psInitPublish(RawMsg, BATTERY);
            RawMsg.batteryPayload.volts = (uint16_t) batteryVolts * 10;
            RawMsg.batteryPayload.amps  = (int16_t) batteryAmps * 10;
            RawMsg.batteryPayload.maxAmps = (int16_t) maxAmps * 10;
            RawMsg.batteryPayload.status = batteryStatus;
            RawMsg.batteryPayload.ampHours = (int16_t) batteryAh * 10;
            RawMsg.batteryPayload.percentage = (uint8_t)(battCapacity - batteryAh) * 100.0 / battCapacity;
            psSendMessage(RawMsg);

            maxAmps = 0;
            
            //Raw analog reports for debug
            if (rawProxReport) {
                //raw prox channels
                for (i = 0; i < PROX_COUNT; i++) {
                    switch (proxDeviceType[i]) {
                        default:
                            continue;
                            break;
                        case BUMPER_FSR:
                            break;
                        case ANALOG_IR:
                        case DIGITAL_IR:
                            if (!IRProxPowered) continue;
                            break;
                    }

                    psInitPublish(RawMsg, FLOAT_DATA);
                    strncpy(RawMsg.nameFloatPayload.name, proxDeviceNames[i], PS_NAME_LENGTH);
                    RawMsg.nameFloatPayload.value = analogSourceVoltages[i];
                    psSendMessage(RawMsg);
                    vTaskDelay(50);
                }
            }

            if (rawAnalogReport) {
                //raw analog channels
                for (i = PROX_COUNT; i < NO_CONECTION; i++) {
                    psInitPublish(RawMsg, FLOAT_DATA);
                    strncpy(RawMsg.nameFloatPayload.name, amuxSourceNames[i - PROX_COUNT], PS_NAME_LENGTH);
                    RawMsg.nameFloatPayload.value = analogSourceVoltages[i];
                    psSendMessage(RawMsg);
                    vTaskDelay(50);
                }
            }

            environmentNextReportTime = xTaskGetTickCount() + envReport;
        }
        vTaskDelayUntil(&xLastWakeTime, ANALOG_TASK_FRQUENCY); //yield
    }
}

void SetMux(int _mux) {
//    int _mux = 13;
    //set the external analog multiplexors
    //set from ISR
    if (_mux & 0x1) {
        PORTSetBits(PMUX0_IOPORT, PMUX0_BIT);
        PORTSetBits(AMUX0_IOPORT, AMUX0_BIT);
    } else {
        PORTClearBits(PMUX0_IOPORT, PMUX0_BIT);
        PORTClearBits(AMUX0_IOPORT, AMUX0_BIT);
    }
    if (_mux & 0x2) {
        PORTSetBits(PMUX1_IOPORT, PMUX1_BIT);
        PORTSetBits(AMUX1_IOPORT, AMUX1_BIT);
    } else {
        PORTClearBits(PMUX1_IOPORT, PMUX1_BIT);
        PORTClearBits(AMUX1_IOPORT, AMUX1_BIT);
    }
    if (_mux & 0x4) {
        PORTSetBits(PMUX2_IOPORT, PMUX2_BIT);
        PORTSetBits(AMUX2_IOPORT, AMUX2_BIT);
    } else {
        PORTClearBits(PMUX2_IOPORT, PMUX2_BIT);
        PORTClearBits(AMUX2_IOPORT, AMUX2_BIT);
    }
    if (_mux & 0x8) {
        PORTSetBits(PMUX3_IOPORT, PMUX3_BIT);
        PORTSetBits(AMUX3_IOPORT, AMUX3_BIT);
    } else {
        PORTClearBits(PMUX3_IOPORT, PMUX3_BIT);
        PORTClearBits(AMUX3_IOPORT, AMUX3_BIT);
    }
}

void Evaluate(int source, float volts) {
    int i;

    int proxRange = 0;

    if (source < PROX_COUNT) {
        //proximity device
        uint8_t mask = proxSensorData[source].directionMask;
        //process according to type
        switch (proxDeviceType[source]) {
            case BUMPER_FSR:
                if (volts < initialSourceVoltages[source] * fsrThresh) {
                    proxSensorData[source].currentRange = 1;
                    ProximityReview(source, true);
                } else {
                    //no bumper detection
                    proxSensorData[source].currentRange = 0;
                    ProximityReview(source, false);
                }
                break;
            case ANALOG_IR:
                if (IRProxPowered) {
                    //convert to cm
                    if (volts <= 0.75f) {
                        proxRange = 100; //100cm
                    } else if (volts >= 3.2f) {
                        proxRange = 15; //15cm
                    } else {
                        proxRange = (int) (33.0 / (volts - 0.33f));
                    }
                    proxSensorData[source].currentRange = proxRange;
                    ProximityReview(source, (proxRange < maxRangeIR));
                }
                break;
            default:
                break;
        }

    } else {
        //other analog sources
        switch (source) {
            case REAR_DIGITAL_PROX:
                //3 digital detectors combined by R network
                if (IRProxPowered) {
                    for (i = 0; i < 8; i++) {
                        if (volts > comboThresholds[i]) {
                            source = proxSource[i];
                            if (i & DIGITAL_PROX_LEFT) {
                                proxSensorData[RL_IR_PROX].currentRange = 10;
                                ProximityReview(RL_IR_PROX, true);
                            } else {
                                proxSensorData[RL_IR_PROX].currentRange = 0;
                                ProximityReview(RL_IR_PROX, false);
                            }
                            if (i & DIGITAL_PROX_CENTER) {
                                proxSensorData[RC_IR_PROX].currentRange = 10;
                                ProximityReview(RC_IR_PROX, true);
                            } else {
                                proxSensorData[RC_IR_PROX].currentRange = 0;
                                ProximityReview(RC_IR_PROX, false);
                            }
                            if (i & DIGITAL_PROX_RIGHT) {
                                proxSensorData[RR_IR_PROX].currentRange = 10;
                                ProximityReview(RR_IR_PROX, true);
                            } else {
                                proxSensorData[RR_IR_PROX].currentRange = 0;
                                ProximityReview(RR_IR_PROX, false);
                            }
                            break;
                        }
                    }
                } else {
                    proxSensorData[RL_IR_PROX].currentRange = -1;
                    proxSensorData[RC_IR_PROX].currentRange = -1;
                    proxSensorData[RR_IR_PROX].currentRange = -1;
                }
                break;
            case EXT_TEMP:
                ext_temp = (((volts * 1000.0 - TEMP_0C_MILLIVOLTS)
                        / TEMP_MV_PER_C) * 9.0 / 5.0) + 32;
                break;
            case INT_TEMP:
                int_temp = (((volts * 1000.0 - TEMP_0C_MILLIVOLTS)
                        / TEMP_MV_PER_C) * 9.0 / 5.0) + 32;
                break;
            case BATTERY_AMPS:
                batteryAmps = (volts - initialSourceVoltages[source]) / 0.083f;
                if (batteryAmps > maxAmps) maxAmps = batteryAmps;
                if (batteryAmps < 0 && batteryAh > 0) batteryAh = 0;
                batteryAh += batteryAmps * ANALOG_TASK_FRQUENCY / (60.0 * 60.0 * 1000.0);
                break;
            case AMBIENT_LIGHT:
                ambientLight = volts * 10;
                break;
            case BATTERY_VOLTS:
                batteryVolts = volts / BATTERY_VOLTS_DIVIDER;
                break;
            case SOLAR_VOLTS:
                solarVolts = volts / SOLAR_VOLTS_DIVIDER;
                break;
            case CHARGE_VOLTS:
                chargeVolts = volts / CHARGER_VOLTS_DIVIDER;
                break;
            case MOTOR_INH:
            {
                motorInhibit = (volts < 1.6f ? true : false);
                if (lastMotorInhResult != motorInhibit) {
                    motorInhResultChangeTime = xTaskGetTickCount();
                    lastRainResult = motorInhibit;
                } else if (motorInhResultChangeTime + MOTOR_INH_DELAY < xTaskGetTickCount()) {
                    //waited long enough
                    if (motorInhibit && !motorInhNotified) {
                        SetCondition(MOTORS_INHIBIT);
                        motorInhNotified = true;
                    } else if (!motorInhibit && motorInhNotified) {
                        CancelCondition(MOTORS_INHIBIT);
                        motorInhNotified = false;
                    }
                }
            }
                break;
            case SOLAR_AMPS:
                solarAmps = (volts - initialSourceVoltages[source]) / AMPS_RANGE_FACTOR; //NB Scale Amps
                break;
            case CHARGE_AMPS:
                chargeAmps = (volts - initialSourceVoltages[source]) / AMPS_RANGE_FACTOR; //NB Scale Amps
                break;
            case RAIN_DIGITAL:
                isRaining = (volts < 1.6f ? true : false);
                if (lastRainResult != isRaining) {
                    rainResultChangeTime = xTaskGetTickCount();
                    lastRainResult = isRaining;
                } else if (rainResultChangeTime + RAIN_DELAY < xTaskGetTickCount()) {
                    //waited long enough
                    if (isRaining && !rainNotified) {
                        SetCondition(RAINING);
                        rainNotified = true;
                    } else if (!isRaining && rainNotified) {
                        CancelCondition(RAINING);
                        rainNotified = false;
                    }
                }
                break;
            default:
                break;
        }
    }

}
//Analog ISR
void __attribute__((interrupt(ADC_IPL), vector(_ADC_VECTOR))) ADC_ISR_Wrapper(void);

void ADC_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    int i;
    unsigned int p = 0;
    unsigned int a = 0;

    for (i = 0; i < (MULTISAMPLE * 2); i += 2) {
        p += ADC1BUF0 + i;
        a += ADC1BUF1 + i;
    }
    pmuxSample[mux] = p / MULTISAMPLE;
    amuxSample[mux] = a / MULTISAMPLE;
    mux++;
    if (mux >= MUX_COUNT) {
        //end of one pass through the prox mux. Flag the process.
        mux = 0;
        xSemaphoreGiveFromISR(doneSemaphore, &higherPriorityTaskWoken);
    }
    //switch the mux
    SetMux(mux);

    INTClearFlag(INT_AD1);
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}

//environmental measurements, declared extern in MCP.h
float ext_temp, int_temp, humidity, ambientLight, rainDetectorAnalog,
batteryVolts, batteryAmps, maxAmps, batteryAh, solarVolts, solarAmps, chargeVolts, chargeAmps;
bool motorInhibit, isRaining;
