/*
 * File:   UARTBroker.c
 * Author: martin
 *
 * Created on November 22, 2013
 */

//Forwards PubSub messages via UART to other MCUs

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

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
#include "PubSubParser.h"

#include "Helpers.h"
#include "Drivers/Serial/Serial.h"
#include "Drivers/Config/Config.h"

//list of UARTS in use
static int uarts[PS_UARTS_COUNT] = PS_UARTS;
//list of baudrates
static int bauds[PS_UARTS_COUNT] = PS_UART_BAUDRATES;
//list of operating modes
static int uartModes[PS_UARTS_COUNT] = PS_UART_MODES;
//connected to...
static char *linkDestinations[PS_UARTS_COUNT] = PS_LINK_DESTINATIONS;
static Subsystem_enum linkSubsystemCodes[PS_UARTS_COUNT] = PS_LINK_SOURCES;

PubSubQueue_t linkQueues[PS_UARTS_COUNT]; //Queues for messages to be sent over the links

bool uartOnline[PS_UARTS_COUNT];
TimerHandle_t uartTimeout[PS_UARTS_COUNT];

extern bool *onlineCondition[PS_UARTS_COUNT];

Condition_enum congestionCondition[PS_UARTS_COUNT] = PS_CONGESTION_CONDITION;
Condition_enum errorsCondition[PS_UARTS_COUNT] = PS_ERRORS_CONDITION;

int uartQueueLimits[] = UART_QUEUE_LIMITS;

void uartOfflineTimerCallback(TimerHandle_t xTimer);

//counts for throttling logging messages
int uartRoutine[PS_UARTS_COUNT];
int uartInfo[PS_UARTS_COUNT];
int uartWarning[PS_UARTS_COUNT];
int uartError[PS_UARTS_COUNT];

int checksum[UART_NUMBER_OF_MODULES];

#define MAX_UART_MESSAGE UART_BROKER_BUFFER_SIZE

//Rx and TX UART tasks - one each per UART
static void psUARTRxTask(void *pvParameters);
static void psUARTTxTask(void *pvParameters);

//PubSubQueue_t bLinkQueues[PS_UARTS_COUNT]; //Queues for messages to be sent over the links
//#define QCHECK configASSERT(bLinkQueues[0] == linkQueues[0]);
#define QCHECK

//UART access functions
void UART_write(UART_MODULE uart, uint8_t dat); //calcs checksum
void UART_writeAddress(UART_MODULE uart);
void UART_watchForAddress(UART_MODULE uart);

//statistics collection
TickType_t lastClearedTime[PS_UARTS_COUNT];
//TX
int messagesSent[PS_UARTS_COUNT];
int addressDiscarded[PS_UARTS_COUNT];
int congestionDiscarded[PS_UARTS_COUNT];      //congestion
int logMessagesDiscarded[PS_UARTS_COUNT];
int sendErrors[PS_UARTS_COUNT];
//RX
int messagesReceived[PS_UARTS_COUNT];
int addressIgnored[PS_UARTS_COUNT];        //wrong address
int parseErrors[PS_UARTS_COUNT];
int receiveErrors[PS_UARTS_COUNT];

bool UARTProcessMessage(Subsystem_enum sourceCode, psMessage_t *msg, TickType_t wait) {
    int i;
    
#ifdef TX_LOCAL_MSG_ONLY
            if (msg->header.source != THIS_SUBSYSTEM) {
                //it's not locally generated, don't send
                return false;
            }
#endif

    for (i = 0; i < PS_UARTS_COUNT; i++) {
        //examine each UART channel
#ifdef TX_DONT_ECHO_MSG
        if (msg->header.source == linkSubsystemCodes[i]) {
            //it's one I received, don't send it back
            addressDiscarded[i]++;
            continue;
        }
#endif
#ifdef DONT_ECHO_APP_OVM
        if ((msg->header.source == APP_OVM) && (linkSubsystemCodes[i] == OVERMIND)) {
            //it's one I received, don't send it back
            addressDiscarded[i]++;
            continue;
        }
#endif

        if (!linkQueues[i]) continue;

        if ((sourceCode != 0) && (sourceCode != linkSubsystemCodes[i])) continue;

        if (msg->header.messageType == SYSLOG_MSG) {
            //count queued log messages and limit
            switch (msg->logPayload.severity) {
                case SYSLOG_ROUTINE:
                    if (uartRoutine[i] > (int)maxUARTRoutine) {
                        logMessagesDiscarded[i]++;
                        continue;
                    }
                    ++uartRoutine[i];
                    break;
                case SYSLOG_INFO:
                    if (uartInfo[i] > (int)maxUARTInfo) {
                        logMessagesDiscarded[i]++;
                        continue;
                    }
                    ++uartInfo[i];
                    break;
                case SYSLOG_WARNING:
                    if (uartWarning[i] > (int)maxUARTWarning) {
                        logMessagesDiscarded[i]++;
                        continue;
                    }
                    ++uartWarning[i];
                    break;
                case SYSLOG_ERROR:
                    if (uartError[i] > (int)maxUARTError) {
                        logMessagesDiscarded[i]++;
                        continue;
                    }
                    ++uartError[i];
                default:
                    break;
            }
        }
        //check q space
        int waiting = uxQueueMessagesWaiting(linkQueues[i]);

        if (waiting >= uartQueueLimits[psQOS[msg->header.messageType]]) {
            SetCondition(congestionCondition[i]);
            congestionDiscarded[i]++;
            DebugPrint("%s uart discarded %s", linkDestinations[i], psLongMsgNames[msg->header.messageType]);
        } else {
//            DebugPrint("%s linkQ: %x", linkDestinations[i], linkQueues[i]);            
            if (xQueueSendToBack(linkQueues[i], msg, wait) != pdTRUE) {
                SetCondition(congestionCondition[i]);
                congestionDiscarded[i]++;
                DebugPrint("%s uart lost %s", linkDestinations[i], psLongMsgNames[msg->header.messageType]);
            }
        }

    }
    return true;
}

int UARTBrokerInit() {
    int i;

    for (i = 0; i < PS_UARTS_COUNT; i++) {
        int uart = uarts[i];
        int baudrate = bauds[i];
        char *destination = linkDestinations[i];

        lastClearedTime[i] = xTaskGetTickCount();
        messagesSent[i]         = 0;
        addressDiscarded[i]     = 0;
        congestionDiscarded[i]  = 0;      //congestion
        logMessagesDiscarded[i] = 0;
        sendErrors[i]           = 0;
        messagesReceived[i]     = 0;
        addressIgnored[i]       = 0;        //wrong address
        parseErrors[i]          = 0;
        receiveErrors[i]        = 0;
  
        //init log message counts
        uartRoutine[i] = uartInfo[i] = uartWarning[i] = uartError[i] = 0;

        //create TX queue for this uart
        if ((linkQueues[i] = psNewPubSubQueue(UART_QUEUE_LENGTH)) == NULL) {
            LogError("UART%i Q", uart + 1);
            SetCondition(errorsCondition[i]);
            return -1;
        }

        //initialize the uart
        switch (uartModes[i]) {
            case PS_UART_9BIT:
            case PS_UART_ASCII9:
                if (!Serial_begin(uart, baudrate,
                        UART_DATA_SIZE_9_BITS | UART_PARITY_NONE | UART_STOP_BITS_1,
                        UART_BROKER_BUFFER_SIZE, UART_BROKER_BUFFER_SIZE))
                {
                    LogError("UART%i begin", uart+1);
                    SetCondition(errorsCondition[i]);
                    return -1;
                }
                break;
            default:
                if (!Serial_begin(uart, baudrate,
                        UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1,
                        UART_BROKER_BUFFER_SIZE, UART_BROKER_BUFFER_SIZE))
                {
                    LogError("UART%i begin", uart+1);
                    SetCondition(errorsCondition[i]);
                    return -1;
                }
               break;
        }


        //create the offline timer
        uartTimeout[i] = xTimerCreate("Offline", // Just a text name, not used by the kernel.
            OFFLINE_TIMER_PERIOD, // The timer period in ticks.
            pdFALSE, // The timer will auto-reload itself when it expires.
            (void *) 0,
            uartOfflineTimerCallback
            );

        //create two tasks for each UART
        char taskName[configMAX_TASK_NAME_LEN];

        snprintf(taskName, configMAX_TASK_NAME_LEN, "U%i Tx %s", uart + 1, destination);
        /* Create the UART Tx task */
        if (xTaskCreate(psUARTTxTask, /* The function that implements the task. */
                (signed char *) taskName, /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                UART_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
                (void *) i, /* The parameter passed to the task. */
                UART_TASK_PRIORITY, /* The priority assigned to the task. */
                NULL) /* The task handle is not required, so NULL is passed. */
                != pdPASS)
        {
            LogError("UART%i Tx", uart + 1);
            SetCondition(errorsCondition[i]);
            return -1;
        }

        snprintf(taskName, configMAX_TASK_NAME_LEN, "U%i Rx %s", uart + 1, destination);
        /* Create the UART Rx task */
        if (xTaskCreate(psUARTRxTask, /* The function that implements the task. */
                (signed char *) taskName, /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                UART_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
                (void *) i, /* The parameter passed to the task. */
                UART_TASK_PRIORITY, /* The priority assigned to the task. */
                NULL) /* The task handle is not required, so NULL is passed. */
                != pdPASS)
        {
            LogError("UART%i Rx", uart + 1);
            SetCondition(errorsCondition[i]);
            return -1;
        }

        uartOnline[i] = false;
    }
    return 0;
}

//Tx task

static void psUARTTxTask(void *pvParameters) {
    int myIndex = (int) pvParameters;
    int myUART = uarts[myIndex];
    char *myDestination= linkDestinations[myIndex];
    int myMode = uartModes[myIndex];
    Subsystem_enum myDestinationSubsystem = linkSubsystemCodes[myIndex];

    psMessage_t msg;    //message to send
    uint8_t msgSequence = 0;
    uint8_t *serialMsg = 0;    //for ASCII translation
    uint8_t *buffer = 0;
    uint8_t size = 0;
    BOOL use9bits;
    
    switch (myMode) {
        case PS_UART_9BIT:
            use9bits = TRUE;
            serialMsg = 0;
            break;
        case PS_UART_ASCII:
            use9bits = FALSE;
            serialMsg = pvPortMalloc(MAX_UART_MESSAGE);
            break;
        case PS_UART_ASCII9:
            use9bits = TRUE;
            serialMsg = pvPortMalloc(MAX_UART_MESSAGE);
            break;
        default:
            serialMsg = 0;
            use9bits = FALSE;
            break;
    }

    DebugPrint("UART%i up, TX to %s", myUART + 1, myDestination);

    for (;;) {
            QCHECK

        //wait for a message
        if (xQueueReceive(linkQueues[myIndex], &msg, portMAX_DELAY) == pdTRUE) {

            if (msg.header.messageType == SYSLOG_MSG) {
                //count down log messages sent
                switch (msg.logPayload.severity) {
                    case SYSLOG_ROUTINE:
                        uartRoutine[myIndex]--;
                        break;
                    case SYSLOG_INFO:
                        uartInfo[myIndex]--;
                        break;
                    case SYSLOG_WARNING:
                        uartWarning[myIndex]--;
                        break;
                    default:
                        uartError[myIndex]--;
                        break;
                }
            } else {
#ifdef UART_DEBUG_LOG
                if (msg.header.messageType != TICK_1S) {
                    LogRoutine("UART Tx: %s >> %s",
                            psLongMsgNames[msg.header.messageType],
                            myDestination);
                }
#endif
            }

    QCHECK

            if (use9bits) UART_writeAddress(myUART);

            switch (myMode) {
                case PS_UART_ASCII:
                case PS_UART_ASCII9:
                    //sending in ASCII
                    msgToText(&msg, serialMsg, MAX_UART_MESSAGE);
                    buffer = (char*) serialMsg;
                    size = strlen(serialMsg);

                    while (size) {
                        UART_write(myUART, *buffer);
                        buffer++;
                        size--;
                    }
                    break;
                default:
                    //sending binary
                    //send STX
                    UART_write(myUART, STX_CHAR);
                    checksum[myUART] = 0; //checksum starts from here
                    //send header
                    UART_write(myUART, msg.header.length);
                    UART_write(myUART, ~msg.header.length);
                    UART_write(myUART, msgSequence++);
                    UART_write(myUART, msg.header.source);
                    UART_write(myUART, msg.header.messageType);
                    UART_write(myUART, msg.header.messageTopic);
                    //send payload
                    buffer = (uint8_t *) &msg.packet;
                    size = msg.header.length;

                    if (size > sizeof(psMessage_t) - SIZEOF_HEADER)
                    {
                        size = msg.header.length = sizeof(psMessage_t) - SIZEOF_HEADER;
                        DebugPrint("Msg Len %i", msg.header.messageType);
                    }

                    while (size) {
                        UART_write(myUART, *buffer);
                        buffer++;
                        size--;
                    }
                    //write checksum
                    UART_write(myUART, (checksum[myUART] & 0xff));
                    
                    messagesSent[myIndex]++;
                    
                        QCHECK

                    break;
            }

        }
    }
}

static char *parseErrorsText[] = PARSE_RESULTS;

static void psUARTRxTask(void *pvParameters) {
    int myIndex;
    int myUART;
    Subsystem_enum mySource;
    char *myDestination;
    int myMode;
    BOOL use9bits;

    char *buffer;
    char *serialMsg;
    int size;

    status_t parseStatus;
    psMessage_t msg;

    ParseResult_enum result;
   
    myIndex = (int) pvParameters;
    myUART = uarts[myIndex];
    myDestination = linkDestinations[myIndex];
    mySource = linkSubsystemCodes[myIndex];
    myMode = uartModes[myIndex];

    switch (myMode) {
        case PS_UART_9BIT:
            use9bits = TRUE;
            serialMsg = 0;
            break;
        case PS_UART_ASCII:
            use9bits = FALSE;
            serialMsg = pvPortMalloc(MAX_UART_MESSAGE);
            break;
        case PS_UART_ASCII9:
            use9bits = TRUE;
            serialMsg = pvPortMalloc(MAX_UART_MESSAGE);
            break;
        default:
            serialMsg = 0;
            use9bits = FALSE;
            break;
    }

    LogRoutine("UART%i up, RX from %s", myUART + 1, myDestination);

    parseStatus.noCRC       = 0; ///< Do not expect a CRC, if > 0
    parseStatus.noSeq       = 0; ///< Do not check seq #s, if > 0
    parseStatus.noLength2   = 0; ///< Do not check for duplicate length, if > 0
    parseStatus.noTopic     = 0; ///< Do not check for topic ID, if > 0
    ResetParseStatus(&parseStatus);

    for (;;) {
            QCHECK

        //wait for address if using 9-bits
        if (use9bits) UART_watchForAddress(myUART);

        switch (myMode) {
            case PS_UART_ASCII:
            case PS_UART_ASCII9:
                //read ascii and convert afterwards
                buffer = serialMsg;
                size = (MAX_UART_MESSAGE - 1);
                while (size) {
                    UART_DATA dat = Serial_read(myUART);
                    *buffer = dat.data8bit;
                    //check for ascii terminator

                    if (*buffer == '\n' || *buffer == '\0') size = 1;
                    buffer++;
                    size--;
                }
                //convert to binary
                *buffer = '\0';
                if (!textToMsg(serialMsg, &msg)) continue;
                break;
            default:
                //binary, read into msg structure

                do {
                    UART_DATA dat = Serial_read(myUART);
                    result = ParseNextCharacter(dat.data8bit, &msg, &parseStatus);

                    switch (result) {
                        case PARSE_OK:
                        case PARSED_MESSAGE:
                            break;
                        default:
                            //error
                            DebugPrint("Parse error: %s", parseErrorsText[result]);
                            if (result != PARSE_SEQUENCE_ERROR || messagesReceived > 0) //ignore first sequence error
                            {
                                parseErrors[myIndex]++;
                                SetCondition(errorsCondition[myIndex]);
                            }
                            result = PARSE_OK;
                            break;
                    }     
                } while (result == PARSE_OK);

                break;
        }

#ifdef UART_DEBUG_LOG
        if (msg.header.messageType != SYSLOG_MSG        //don't log log messages!!
                && msg.header.messageType != TICK_1S)   //& don't log ticks
            LogRoutine("UART Rx: %s << %s",
                psLongMsgNames[msg.header.messageType], myDestination);
#endif
#ifdef RX_SOURCE_MSG_ONLY
        if (msg.header.source != mySource) {
            //it's not one originated by my source
            addressIgnored[myIndex]++;
            continue;
        }
#endif
        if (msg.header.source == THIS_SUBSYSTEM) {
            //it's locally generated
            addressIgnored[myIndex]++;
            continue;
        }

        //route the message
        psForwardMessage(&msg, 0);

        messagesReceived[myIndex]++;
        
        if (!uartOnline[myIndex])
        {
            *onlineCondition[myIndex] = true;
            uartOnline[myIndex] = true;
        }
        xTimerReset(uartTimeout[myIndex], 0);
    }
}

void UART_write(UART_MODULE UART, uint8_t dat) {
    checksum[UART] += dat;
    UART_DATA udat;
    udat.__data = 0;
    udat.data8bit = dat;
    Serial_writeData(UART, udat);
}

void UART_writeAddress(UART_MODULE UART) {
    UART_DATA dat;
    dat.data9bit = PS_UART_ADDRESS | 0x100;
    Serial_writeData(UART, dat);
}

void UART_watchForAddress(UART_MODULE UART) {
    //use the UART feature. Return immediately
    Serial_watch_for_address(UART, PS_UART_ADDRESS);
}

void uartOfflineTimerCallback(TimerHandle_t xTimer)
{
    int i;

    for (i=0; i<PS_UARTS_COUNT; i++)
    {
        if (uartTimeout[i] == xTimer)
        {
            if (uartOnline[i])
            {
                *onlineCondition[i] = false;
                uartOnline[i] = false;
                return;
            }
        }
    }
}

void UARTSendStats()
{
    psMessage_t msg;
    int i;
            
    for (i=0; i<PS_UARTS_COUNT; i++)\
    {
        psInitPublish(msg, COMMS_STATS);
    
        strncpy(msg.commsStatsPayload.destination, linkDestinations[i], 4);
        msg.commsStatsPayload.messagesSent          = messagesSent[i];
        msg.commsStatsPayload.addressDiscarded      = addressDiscarded[i];
        msg.commsStatsPayload.congestionDiscarded   = congestionDiscarded[i]; //congestion
        msg.commsStatsPayload.logMessagesDiscarded  = logMessagesDiscarded[i];
        msg.commsStatsPayload.sendErrors            = sendErrors[i];
        msg.commsStatsPayload.messagesReceived      = messagesReceived[i];
        msg.commsStatsPayload.addressIgnored        = addressIgnored[i];        //wrong address
        msg.commsStatsPayload.parseErrors           = parseErrors[i];        //wrong address
        msg.commsStatsPayload.receiveErrors         = receiveErrors[i];
        
        psSendPubSubMessage(&msg);
        
        vTaskDelay(50);
    }
}

void UARTResetStats()
{
    int i;
            
//    for (i=0; i<PS_UARTS_COUNT; i++)\
//    {
//        TickType_t now = xTaskGetTickCount();
//
//        messagesSent[i]         = 0;
//        addressDiscarded[i]     = 0;
//        congestionDiscarded[i]  = 0;      //congestion
//        logMessagesDiscarded[i] = 0;
//        messagesReceived[i]     = 0;
//        addressIgnored[i]       = 0;        //wrong address
//        parseErrors[i]          = 0;
//        
//        lastClearedTime[i] = now;
//    }
}