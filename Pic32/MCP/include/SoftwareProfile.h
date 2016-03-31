/* 
 * File:   SoftwareProfile.h
 * Author: martin
 *
 * SW Config file for MCP.
 *
 * Created on November 19, 2013, 5:43 PM
 */

#ifndef SOFTWAREPROFILE_H
#define	SOFTWAREPROFILE_H

#include "syslog/Logging_levels.h"
#include "pubsub/Serial_Broker_Modes.h"

#define THIS_SUBSYSTEM              MCP_SUBSYSTEM
#define NOTIFICATION_CENTRAL

#define TX_DONT_ECHO_MSG        //don't send message back to source
//#define RX_SOURCE_MSG_ONLY      //only process received messages from source
#define DONT_ECHO_APP_OVM

#define BLE_DEBUG_LOG
//#define UART_DEBUG_LOG
#define XBEE_DEBUG_LOG

//UART handler interrupt priorities
#define UART_INT_PRIORITY           INT_PRIORITY_LEVEL_3
#define UART_INT_SUB_PRIORITY       INT_SUB_PRIORITY_LEVEL_0
#define UART_IPL                    ipl3soft

//Logging levels
#define LOG_TO_SERIAL               LOG_ALL         //printed log
#define SYSLOG_LEVEL                LOG_INFO_PLUS   //published log

//*********************MCP Subsystems

#define SUBSYSTEM_RTOS_TASKS        20

//task to respond to App messages
#define MCP_TASK_PRIORITY           ( 3 )
#define MCP_TASK_STACK_SIZE         (700)
#define MCP_TASK_QUEUE_LENGTH       20

#define POWERUP_TIMER_PERIOD        5000
#define POWEROFF_TIMER_PERIOD       5000
#define POWERUP_RETRIES             5
//1 second ticks
#define TICK_TASK_STACK_SIZE        300
#define TICK_TASK_PRIORITY          2

//I2C4_Task
#define I2C4_TASK_STACK_SIZE        300
#define I2C4_TASK_PRIORITY          2

//task to measure volts and amps
#define ANALOG_TASK_STACK_SIZE      400
#define ANALOG_TASK_PRIORITY        5
#define ADC_IPL                     ipl4soft
#define ADC_INT_PRIORITY            ADC_INT_PRI_4
#define ADC_INT_SUB_PRIORITY        ADC_INT_SUB_PRI_0
#define ANALOG_TASK_FRQUENCY        100

#define PROXIMITY_TASK_STACK_SIZE   300
#define PROXIMITY_TASK_PRIORITY     ( 5 )

//sonar I2C task
#define I2C1_TASK_STACK_SIZE        250
#define I2C1_TASK_PRIORITY          3
#define USING_I2C_1                 1
#define SONAR_I2C                   I2C1

//GPIO I2C
#define GPIO_I2C		    I2C4
#define USING_I2C_4                 1
#define PIR_START_WAIT              30      //seconds

#define I2C_CLOCK_FREQ              100000
#define I2C_INT_PRIORITY            INT_PRIORITY_LEVEL_2
#define I2C_INT_SUB_PRIORITY        INT_SUB_PRIORITY_LEVEL_0
#define I2C_IPL                     ipl2soft
#define I2C_SEMPHR_WAIT             100     //timeout

//*******************PUBSUB Subsystem

//PubSub broker
#define PS_QUEUE_LENGTH             30
#define PS_TASK_PRIORITY            5
#define PS_TASK_STACK_SIZE          300
#define QOS_WAIT_TIMES              {1000, 250, 0}
#define NOTIFY_TASK_STACK_SIZE      300
#define NOTIFY_TASK_PRIORITY        5
#define NOTIFY_TASK_QUEUE_LENGTH    20
#define CONDITION_TASK_STACK_SIZE   300
#define CONDITION_TASK_PRIORITY     ( 5 )
#define CONDITION_REPORT_INTERVAL   250

//BLE Handler
//#define PS_BLE_BROKER
#define BLE_UART                    UART1
#define BLE_BAUDRATE                57600
#define USING_UART_1A                1
#define BLE_MESSAGE_LEN             25
#define BLE_TASK_PRIORITY           ( 4 )
#define BLE_TASK_STACK_SIZE         300
#define BLE_QUEUE_LENGTH            150
#define BLE_QUEUE_LIMITS           {BLE_QUEUE_LENGTH, BLE_QUEUE_LENGTH*0.8, BLE_QUEUE_LENGTH*0.5}
#define BLE_BROKER_BUFFER_SIZE      32
#define BLE_OFFLINE_TIMER_PERIOD    60000

//XBEE Broker
#define XBEE_BROKER
#define PAN_ID                      0x3332
#define MCP_ADDRESS                 0x5A
#define PROXY_ADDRESS               0x1A
#define XBEE_UART				UART2B
#define XBEE_BAUDRATE       	115200
#define XBEE_QUEUE_LENGTH           50
#define USING_UART_2B                1
#define XBEE_TASK_STACK_SIZE       300
#define XBEE_TASK_PRIORITY         ( 4 )
#define XBEE_QUEUE_LIMITS           {XBEE_QUEUE_LENGTH, XBEE_QUEUE_LENGTH * 0.8, XBEE_QUEUE_LENGTH * 0.5}
#define XBEE_BROKER_BUFFER_SIZE     100

//UART Broker
#define PS_UART_BROKER
#define USING_UART_3A                1
#define USING_UART_3B                1
#define PS_UARTS_COUNT               2
#define PS_UARTS                {UART3A, UART3B}
#define PS_UART_BAUDRATES       {115200, 115200}
#define PS_UART_MODES           {PS_UART_8BIT, PS_UART_8BIT}
#define PS_LINK_DESTINATIONS    {"MOT", "OVM"}
#define PS_LINK_SOURCES         {MOTOR_SUBSYSTEM, OVERMIND}
#define PS_CONGESTION_CONDITION {MCP_MOT_COMMS_CONGESTION, MCP_OVM_COMMS_CONGESTION}
#define PS_ERRORS_CONDITION     {MCP_MOT_COMMS_ERRORS, MCP_OVM_COMMS_ERRORS}
#define UART_QUEUE_LENGTH           100

#define OFFLINE_TIMER_PERIOD        60000
#define UART_TASK_STACK_SIZE       300
#define UART_TASK_PRIORITY         ( 4 )
#define PS_UART_ADDRESS             0xAA    //message delimiter
#define UART_QUEUE_LIMITS           {UART_QUEUE_LENGTH, UART_QUEUE_LENGTH * 0.8, UART_QUEUE_LENGTH * 0.5}
#define UART_BROKER_BUFFER_SIZE     64
//*******************


//******************Logging
#define LOG_TASK_STACK_SIZE         300
#define LOG_TASK_PRIORITY           ( 2 )
#define SERIAL_LOG_QUEUE_LENGTH     30

#ifndef XBEE_BROKER
#define DEBUG_PRINT
#define LOG_UART                    UART2B
#define LOG_UART_BAUDRATE           (57600)
#define USING_UART_2B_TX             1
#endif
#ifndef PS_BLE_BROKER
#define DEBUG_PRINT
#define LOG_UART                    UART1
#define LOG_UART_BAUDRATE           (57600)
#define USING_UART_1A_TX             1
#endif

#endif	/* SOFTWAREPROFILE_H */

