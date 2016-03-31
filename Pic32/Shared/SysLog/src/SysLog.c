/*
 * File:   SysLog.c
 * Author: martin
 *
 * Created on November 17, 2013, 6:17 PM
 */

//System wide logging facility.

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "FreeRTOS.h"

#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "PubSub/PubSub.h"
#include "PubSubData.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "Drivers/Serial/Serial.h"

#define _SUPPRESS_PLIB_WARNING
#include <plib.h>

#include "SysLog/SysLog.h"

//simple structure to send direct to serial task on dedicated queue

typedef struct {
    uint8_t severity;
    char text[PS_MAX_LOG_TEXT];
} SerialLog_t;

QueueHandle_t serialLogQ;

#define LOG_MESSAGE_SIZE 80
char logMessage[LOG_MESSAGE_SIZE]; //printable message

void Syslog_print(psMessage_t *msg);
//called by tasks to log a message

#ifdef DEBUG_PRINT
SemaphoreHandle_t degugSerialMutex; //mutex for the serial port
#endif

void _LogMessage(SysLogSeverity_enum _severity, const char *_message) {
    //send to the Logger task
    psMessage_t msg;
    psInitPublish(msg, SYSLOG_MSG);
    msg.logPayload.severity = _severity;
    strncpy(msg.logPayload.text, _message, PS_MAX_LOG_TEXT);
    xQueueSendToBack(serialLogQ, (void*) &msg, 0); //don't wait or error if Q full
}

//Serial logging task
void SerialLogTask(void *params);

//start the logging task

int SysLogInit() {

#ifdef DEBUG_PRINT

    //config the Log UART - not using interrupts
    INTEnable(INT_U1E + LOG_UART, INT_DISABLED);
    INTEnable(INT_U1RX + LOG_UART, INT_DISABLED);
    INTEnable(INT_U1TX + LOG_UART, INT_DISABLED);

    UARTConfigure(LOG_UART, UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetLineControl(LOG_UART, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(LOG_UART, GetPeripheralClock(), LOG_UART_BAUDRATE);
    UARTEnable(LOG_UART, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_TX)); //note, no Rx

    degugSerialMutex = xSemaphoreCreateMutex();

#endif
    if ((serialLogQ = xQueueCreate(SERIAL_LOG_QUEUE_LENGTH, sizeof (psMessage_t))) == NULL) {

#ifdef DEBUG_PRINT
        Syslog_write_string("\nSerial Log failed to create Queue\n");
#endif
        return -1;
    } else {
        /* Create the logger task */
        if (xTaskCreate(SerialLogTask, /* The function that implements the task. */
                (signed char *) "Logger", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                LOG_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
                (void *) 0, /* The parameter passed to the task. */
                LOG_TASK_PRIORITY, /* The priority assigned to the task. */
                NULL) /* The task handle is not required, so NULL is passed. */
                != pdTRUE)
        {
            DebugPrint("No Serial Log Task");
            SetCondition(PIC_INIT_ERROR);
            return -1;
        }
    }
    return 0;
}


void SerialLogTask(void *params) {
#ifdef DEBUG_PRINT
    Syslog_write_string("\n****************** Serial Log Opened\n");
#endif
    //now, wait for messages
    for (;;) {
        psMessage_t msg;

        if (xQueueReceive(serialLogQ, &msg, portMAX_DELAY) == pdTRUE) {

            msg.header.source = THIS_SUBSYSTEM;

            int minLogLevel;

            switch (msg.logPayload.severity) {

                case SYSLOG_ROUTINE:
                    minLogLevel = LOG_ALL;
                    break;
                case SYSLOG_INFO:
                    minLogLevel = LOG_INFO_PLUS;
                    break;
                case SYSLOG_WARNING:
                    minLogLevel = LOG_WARN_PLUS;
                    break;
                case SYSLOG_ERROR:
                case SYSLOG_FAILURE:
                default:
                    minLogLevel = LOG_ERROR_ONLY;
                    break;
            }

            if (SYSLOG_LEVEL <= minLogLevel) {
                psForwardMessage(&msg, 0);
            }

#ifdef DEBUG_PRINT
            if (LOG_TO_SERIAL <= minLogLevel) {
                Syslog_print(&msg);
            }
#endif
        }
    }
}

#ifdef DEBUG_PRINT

void Syslog_print(psMessage_t *msg)
{
    char *severity;
    uint8_t code;
    char *source = subsystemNames[THIS_SUBSYSTEM];

    switch (msg->logPayload.severity) {
        case SYSLOG_ROUTINE:
            severity = "    ";
            break;
        case SYSLOG_INFO:
            severity = "INFO";
            break;
        case SYSLOG_WARNING:
            severity = "WARN";
            break;
        case SYSLOG_ERROR:
            severity = "EROR";
            break;
        case SYSLOG_FAILURE:
            severity = "FAIL";
            break;
        default:
            severity = "????";

            break;
    }

    snprintf(logMessage, LOG_MESSAGE_SIZE, "%6li %s@%s: %s", xTaskGetTickCount(), severity, source,
            msg->logPayload.text);
    Syslog_write_string(logMessage);
}

void Syslog_write(uint8_t theChar) {

    while (!UARTTransmitterIsReady(LOG_UART))
        vTaskDelay(1);
    UARTSendDataByte(LOG_UART, theChar);
}

//write a null-terminated string

void Syslog_write_string(const unsigned char *buffer) {

    xSemaphoreTake(degugSerialMutex, portMAX_DELAY);

    int size = strlen(buffer);
    while (size) {
        Syslog_write(*buffer);
        buffer++;
        size--;
    }
    Syslog_write((uint8_t)'\n');
    while (!UARTTransmissionHasCompleted(LOG_UART)) {
        vTaskDelay(1);
    }
    xSemaphoreGive(degugSerialMutex);
}

void _ExceptionMessage(const unsigned char *buffer) {
    while (*buffer) {
    while (!UARTTransmitterIsReady(LOG_UART));
    UARTSendDataByte(LOG_UART, *buffer);
        buffer++;
    }
    while (!UARTTransmissionHasCompleted(LOG_UART));
}

#else

void _ExceptionMessage(const unsigned char *buffer) {}

#endif