/* 
 * File:   SoftwareProfile.h
 * Author: martin
 *
 * SW Config file for Motor Control MCU.
 *
 * Created on November 19, 2013, 5:43 PM
 */

#ifndef SOFTWAREPROFILE_H
#define	SOFTWAREPROFILE_H

#include "SysLog/Logging_levels.h"
#include "PubSub/Serial_Broker_Modes.h"

#define THIS_SUBSYSTEM          MOTOR_SUBSYSTEM
#define TX_LOCAL_MSG_ONLY
#define UART_DEBUG_LOG

//UART handler interrupt priorities
#define UART_INT_PRIORITY           INT_PRIORITY_LEVEL_3
#define UART_INT_SUB_PRIORITY       INT_SUB_PRIORITY_LEVEL_0
#define UART_IPL                    ipl3soft

//Logging levels
#define LOG_TO_SERIAL               LOG_ALL
#define SYSLOG_LEVEL                LOG_INFO_PLUS

//*********************************MOT Subsystems
#define SUBSYSTEM_RTOS_TASKS        20

#define ENC_INT_PRIORITY            EXT_INT_PRI_6
#define ENC_INT_SUB_PRIORITY        EXT_INT_SUB_PRI_0
#define ENC_IPL                     ipl6soft

#define AMPS_TASK_PRIORITY          ( 3 )
#define AMPS_TASK_STACK_SIZE        (500)
#define AMPS_SENSITIVITY            (110.0f / 1000)      //Volts per Amp
#define ANALOG_VREF                 3.3f        //ADC VDD
#define AMPS_SCALE_FACTOR           ((ANALOG_VREF / 1024) / AMPS_SENSITIVITY)  //Amps per ADC LSB

#define MOT_TASK_PRIORITY           4
#define MOT_TASK_STACK_SIZE         (500)
#define MOT_TASK_QUEUE_LENGTH       100
#define PID_TASK_STACK_SIZE         (1000)
#define PID_TASK_PRIORITY           5
//*********************************PUBSUB Subsystem

//PubSub broker
#define PS_QUEUE_LENGTH             100
#define PS_TASK_PRIORITY            ( 4 )
#define PS_TASK_STACK_SIZE          (1000)
#define QOS_WAIT_TIMES              {portMAX_DELAY, 250, 0}
#define NOTIFY_TASK_STACK_SIZE      500
#define NOTIFY_TASK_PRIORITY        ( 4 )
#define NOTIFY_TASK_QUEUE_LENGTH    50
#define CONDITION_TASK_STACK_SIZE   500
#define CONDITION_TASK_PRIORITY     ( 5 )
#define CONDITION_REPORT_INTERVAL   250

//UART Broker
#define PS_UART_BROKER
#define USING_UART_2A                1
#define PS_UARTS                    {UART2A}
#define PS_UART_BAUDRATES           {115200}
#define PS_UART_MODES               {PS_UART_8BIT}
#define PS_UARTS_COUNT              1
#define PS_LINK_DESTINATIONS        {"MCP"}
#define PS_LINK_SOURCES             {MCP_SUBSYSTEM}
#define PS_CONGESTION_CONDITION {MOT_MCP_COMMS_CONGESTION}
#define PS_ERRORS_CONDITION     {MOT_MCP_COMMS_ERRORS}

#define OFFLINE_TIMER_PERIOD        5000
#define UART_TASK_STACK_SIZE        500
#define UART_TASK_PRIORITY          ( 4 )
#define PS_UART_ADDRESS             0xAA    //message delimeter
#define UART_QUEUE_LENGTH           300
#define UART_QUEUE_LIMITS           {UART_QUEUE_LENGTH, UART_QUEUE_LENGTH / 2, UART_QUEUE_LENGTH / 4}
#define UART_BROKER_BUFFER_SIZE     64


//syslog queueing limits to avoid congestion
#define UART_ROUTINE_MAX             0
#define UART_INFO_MAX                10
#define UART_WARNING_MAX             10
#define UART_ERROR_MAX               10

//Serial logger
#define DEBUG_PRINT
#define LOG_TASK_STACK_SIZE         500
#define LOG_TASK_PRIORITY           ( 2 )
#define SERIAL_LOG_QUEUE_LENGTH     100
#define LOG_UART                    UART2B
#define LOG_UART_BAUDRATE           (57600)
#define USING_UART_2B_TX             1

#endif	/* SOFTWAREPROFILE_H */

