/*
 * File:   AmpsTask.c
 * Author: martin
 *
 * Created on January 14, 2014
 */
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

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "SysLog/SysLog.h"
#include "PubSub/PubSub.h"
#include "PubSubData.h"

#include "Motor.h"

//Task to measure the current usage by the motors
static void AmpsTask(void *pvParameters);

//mapping analog channels to motors
int AmpsChannelsToMotors[4] = AN_TO_MOTORS;

int   AmpsInit() {

    if (xTaskCreate(AmpsTask, /* The function that implements the task. */
            (signed char *) "AMPS Task", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
            AMPS_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
            (void *) 0, /* The parameter passed to the task. */
            AMPS_TASK_PRIORITY, /* The priority assigned to the task. */
            NULL) /* The task handle is not required, so NULL is passed. */
            != pdPASS) {
        LogError("Amps Task");
        SetCondition(MOT_INIT_ERROR);
        return -1;
    }
    return -1;
}

// Define setup parameters for OpenADC10 function
// Turn module on | Ouput in integer format | Trigger mode auto | Enable autosample
#define config1     ADC_FORMAT_INTG | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON | ADC_SAMP_OFF
// ADC ref external | Disable offset test | Disable scan mode | Perform 4 samples |
//      Use dual buffers | Use alternate mode
#define config2     ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_ON | ADC_SAMPLES_PER_INT_4 | \
        ADC_ALT_BUF_ON | ADC_ALT_INPUT_OFF
// Use ADC internal clock | Set sample time
#define config3     ADC_CONV_CLK_INTERNAL_RC | ADC_SAMPLE_TIME_15

static void AmpsTask(void *pvParameters)
{
    //TODO: synchronize ADC to OC

    CloseADC10();   // Ensure the ADC is off before setting the configuration

    SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF );
    OpenADC10(config1, config2, config3, AMPS_CONFIG_SCAN, AMPS_CONFIG_PORT);

    EnableADC10();

    while(1) {
            unsigned int offset;    // Buffer offset to point to the base of the idle buffer
            int i;

            //Todo: Synchronize sampling to PWM

            //start the ADC sampling & then converting
            AcquireADC10();

            while (!mAD1GetIntFlag()) {
                // Wait for the conversion to complete so there will be vaild data in ADC result registers
                vTaskDelay(100);
            }

            // Determine which buffer is idle and create an offset
            offset = 8 * ((~ReadActiveBufferADC10() & 0x01));

            for (i=0; i<4; i++)
            {
                int adc = ReadADC10(offset + i);
                if (adc > 0)
                    motors[AmpsChannelsToMotors[i]].amps = (float) ((adc - motors[AmpsChannelsToMotors[i]].ampsZero) * AMPS_SCALE_FACTOR);
            }
            mAD1ClearIntFlag();
    }
}