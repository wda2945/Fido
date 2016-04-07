/*
 * File:   xbee serialbroker.c
 * Author: martin
 *
 * Created on Feb 7 2016
 */
//PIC32

//Forwards PubSub messages via UART to Proxy

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
#include "semphr.h"
#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "SysLog/SysLog.h"

#include "PubSub/PubSub.h"
#include "PubSubData.h"
#include "PubSubParser.h"

#include "Helpers.h"
#include "Drivers/Serial/Serial.h"
#include "Drivers/Config/Config.h"
#include "MCP.h"
#include "pubsub/notifications.h"

#include "PubSub/xbee.h"

//Message Tx queue
PubSubQueue_t xbeeTxQueue;

int xbeeQueueLimits[3] = XBEE_QUEUE_LIMITS; //by QoS

TimerHandle_t xbeeTimeout;
bool xbeeOnline;
void xbeeOfflineTimerCallback(TimerHandle_t xTimer);

//RX and TX XBee threads
static void XBeeRxTask(void *pvParameters);
static void XBeeTxTask(void *pvParameters);

//regular Fido data packet (Tx)

typedef struct {

    struct {
        uint8_t apiIdentifier;
        uint8_t frameId;
        uint8_t destinationMSB;
        uint8_t destinationLSB;
        uint8_t options;
    } packetHeader;
    psMessage_t message;
} TxPacket_t;

//XBee AT command packet

typedef struct {

    struct {
        uint8_t apiIdentifier;
        uint8_t frameId;
        uint8_t ATcommand[2];
    } packetHeader;

    union {

        struct {
            uint8_t byteData;
            uint8_t fill[3];
        };
        int intData;
    };
} ATPacket_t;

//regular Fido data packet (Rx) + status packets

typedef struct {
    uint8_t apiIdentifier;

    union {

        struct {

            struct {
                uint8_t sourceMSB;
                uint8_t sourceLSB;
                uint8_t rssi;
                uint8_t options;
            } packetHeader;
            psMessage_t message;
        } fidoMessage;

        struct {
            uint8_t frameId;
            uint8_t status;
        } txStatus;

        struct {
            uint8_t status;
        } modemStatus;
    };
} RxPacket_t; //regular Fido data packet (Rx) + status packets

//XBee AT Command Response

typedef struct {
    uint8_t apiIdentifier;
    uint8_t frameId;
    uint8_t ATcommand[2];
    uint8_t status;

    union {

        struct {
            uint8_t byteData;
            uint8_t fill[3];
        };
        int intData;
    };
} ATResponse_t; //XBee AT Command Response

//XBEE Tx Status

struct {
    XBeeTxStatus_enum status;
    uint8_t frameId;
} latestTxStatus;
char *txStatusNames[] = TX_STATUS_NAMES;

XBeeModemStatus_enum latestXBeeModemStatus;

TxPacket_t XBeeTxPacket; //regular Fido data packet (Tx)
ATPacket_t XBeeATPacket; //XBee AT command packet

RxPacket_t XBeeRxPacket; //regular Fido data packet (Rx) + status packets
ATResponse_t XBeeATResponse; //XBee AT Command Response

//semaphore for ADC ISR sync
SemaphoreHandle_t XBeeTxMutex = NULL; //use of Tx Mutex
SemaphoreHandle_t XBeeATResponseSemaphore = NULL; //wait for AT command response
SemaphoreHandle_t XBeeTxResponseSemaphore = NULL; //wait for Tx response

int XBeeTxChecksum;
int XBeeRxChecksum;
uint8_t XBeeTxSequenceNumber = 1;
uint8_t XBeeRxSequenceNumber;

//counts for throttling logging messages
int XBeeRoutine = 0;
int XBeeInfo = 0;
int XBeeWarning = 0;
int XBeeError = 0;

//comms statistics collection
//TX
int XBeeMessagesSent;
int XBeeAddressDiscarded;
int XBeeCongestionDiscarded; //congestion
int XBeeLogMessagesDiscarded;
int XBeeSendErrors;
//RX
int XBeeMessagesReceived;
int XBeeAddressIgnored; //wrong address
int XBeeReceiveErrors;
int XBeeParseErrors;

//register access using API
int APISetXBeeRegister1(const char *atCommand, uint8_t value);
int APIGetXBeeRegister1(const char *atCommand);

//register access using AT command mode
int SetXBeeRegister(const char *atCommand, uint8_t value);
int GetXBeeRegister(const char *atCommand);

//mode switching
int EnterCommandMode();
int EnterAPIMode();

int XBeeBrokerInit() {

    XBeeMessagesSent = 0;
    XBeeAddressDiscarded = 0;
    XBeeCongestionDiscarded = 0; //congestion
    XBeeLogMessagesDiscarded = 0;
    XBeeSendErrors = 0;
    XBeeMessagesReceived = 0;
    XBeeAddressIgnored = 0;
    XBeeReceiveErrors = 0; //wrong address
    XBeeParseErrors = 0;

    //init log message counts
    XBeeRoutine = XBeeInfo = XBeeWarning = XBeeError = 0;

    //create TX queue for the XBee
    if ((xbeeTxQueue = psNewPubSubQueue(XBEE_QUEUE_LENGTH)) == NULL) {
        LogError("XBee Q");
        SetCondition(MCP_XBEE_COMMS_ERRORS);
        return -1;
    }

    if (!Serial_begin(XBEE_UART, XBEE_BAUDRATE,
            UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1,
            XBEE_BROKER_BUFFER_SIZE, XBEE_BROKER_BUFFER_SIZE)) {
        LogError("XBee begin");
        SetCondition(MCP_XBEE_COMMS_ERRORS);
        return -1;
    }

    DebugPrint("xbee uart configured");

    bool xbeeReady = false;
    int i;

    for (i = 0; i < 10; i++) {

        if (EnterCommandMode() < 0) {
            LogError("EnterCommandMode() fail");
            continue;
        }

        int power = GetPowerLevel();
        if (power < 0) {
            LogError("Read PowerLevel fail");
            continue;
        }

        DebugPrint("xbee power level %i", power);

        if (EnterAPIMode() < 0) {
            LogError("EnterAPIMode() fail");
            continue;
        }
        xbeeReady = true;
        break;
    }

    if (xbeeReady) {

        //create the semaphore to sync a Tx with a Tx Status
        XBeeTxResponseSemaphore = xSemaphoreCreateBinary();
        if (XBeeTxResponseSemaphore == NULL) {
            LogError("XBee Tx Semphr");
            SetCondition(MCP_XBEE_COMMS_ERRORS);
            return -1; //outa here
        }
        xSemaphoreTake(XBeeTxResponseSemaphore, 0); //make sure it's empty

        //create the semaphore to sync an AT command with a responese
        XBeeATResponseSemaphore = xSemaphoreCreateBinary();
        if (XBeeATResponseSemaphore == NULL) {
            LogError("XBee AT Semphr");
            SetCondition(MCP_XBEE_COMMS_ERRORS);
            return -1; //outa here
        }
        xSemaphoreTake(XBeeATResponseSemaphore, 0); //make sure it's empty

        //create the Tx access mutex
        XBeeTxMutex = xSemaphoreCreateMutex();
        if (XBeeTxMutex == NULL) {
            LogError("XBee Mutex");
            SetCondition(MCP_XBEE_COMMS_ERRORS);
            return -1; //outa here
        }

        //create the offline timer
        xbeeTimeout = xTimerCreate("Offline", // Just a text name, not used by the kernel.
                OFFLINE_TIMER_PERIOD, // The timer period in ticks.
                pdFALSE, // The timer will not auto-reload itself when it expires.
                (void *) 0,
                xbeeOfflineTimerCallback
                );

        /* Create the XBee Tx task */
        if (xTaskCreate(XBeeTxTask, /* The function that implements the task. */
                "XBee Tx", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                XBEE_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
                (void *) 0, /* The parameter passed to the task. */
                XBEE_TASK_PRIORITY, /* The priority assigned to the task. */
                NULL) /* The task handle is not required, so NULL is passed. */
                != pdPASS) {
            LogError("XBee Tx");
            SetCondition(MCP_XBEE_COMMS_ERRORS);
            return -1;
        }

        /* Create the XBee Rx task */
        if (xTaskCreate(XBeeRxTask, /* The function that implements the task. */
                "XBee Rx", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                XBEE_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
                (void *) 0, /* The parameter passed to the task. */
                XBEE_TASK_PRIORITY, /* The priority assigned to the task. */
                NULL) /* The task handle is not required, so NULL is passed. */
                != pdPASS) {
            LogError("XBee Rx");
            SetCondition(MCP_XBEE_COMMS_ERRORS);
            return -1;
        }

        xbeeOnline = false;
        return 0;
    } else return -1;
}

//called by the broker to see whether a message should be queued for the TX Thread

bool XBeeBrokerProcessMessage(psMessage_t *msg, TickType_t wait) {

    if (msg->header.source == ROBO_APP || msg->header.source == APP_XBEE || msg->header.source == APP_OVM || msg->header.source == XBEE) {
        //it's one I received, don't send it back
        XBeeAddressDiscarded++;
        return false;
    }

    if (!xbeeTxQueue) return false;

    if (msg->header.messageType == SYSLOG_MSG) {
        //count queued log messages and limit
        switch (msg->logPayload.severity) {
            case SYSLOG_ROUTINE:
                if (XBeeRoutine > (int) maxXBeeRoutine) {
                    XBeeLogMessagesDiscarded++;
                    return false;
                }
                ++XBeeRoutine;
                break;
            case SYSLOG_INFO:
                if (XBeeInfo > (int) maxXBeeInfo) {
                    XBeeLogMessagesDiscarded++;
                    return false;
                }
                ++XBeeInfo;
                break;
            case SYSLOG_WARNING:
                if (XBeeWarning > (int) maxXBeeWarning) {
                    XBeeLogMessagesDiscarded++;
                    return false;
                }
                ++XBeeWarning;
                break;
            case SYSLOG_ERROR:
                if (XBeeError > (int) maxXBeeError) {
                    XBeeLogMessagesDiscarded++;
                    return false;
                }
                ++XBeeError;
            default:
                break;
        }
    }
    //check q space
    int waiting = uxQueueMessagesWaiting(xbeeTxQueue);
    if (waiting >= xbeeQueueLimits[psQOS[msg->header.messageType]]) {
        SetCondition(MCP_XBEE_COMMS_CONGESTION);
        XBeeCongestionDiscarded++;
        DebugPrint("XBee discarded %s", psLongMsgNames[msg->header.messageType]);
        return false;
    } else {
        //            DebugPrint("%s linkQ: %x", linkDestinations, linkQueues);            
        if (xQueueSendToBack(xbeeTxQueue, msg, wait) != pdTRUE) {
            SetCondition(MCP_XBEE_COMMS_CONGESTION);
            XBeeCongestionDiscarded++;
            DebugPrint("XBee lost %s", psLongMsgNames[msg->header.messageType]);
            return false;
        }
    }

    return true;
}

void XBEE_write(uint8_t dat) {
    UART_DATA udat;
    udat.__data = 0;
    udat.data8bit = dat;
    Serial_writeData(XBEE_UART, udat);
}

void WriteEscapedByte(uint8_t b) {
    XBeeTxChecksum += b;

    switch (b) {
        case FRAME_DELIMITER:
        case ESCAPE:
        case XON:
        case XOFF:
            XBEE_write(ESCAPEE);
            XBEE_write(b ^ CHAR_XOR);
            break;
        default:
            XBEE_write(b);
            break;
    }
}

void SendApiPacket(uint8_t *pkt, int len) {
    int reply;
    uint8_t *next = pkt;
    int count = len;

    XBEE_write(FRAME_DELIMITER);
    WriteEscapedByte((len >> 8) & 0xff);
    WriteEscapedByte(len & 0xff);

    XBeeTxChecksum = 0;

    do {
        WriteEscapedByte(*pkt++);
        count--;
    } while (count > 0);

    WriteEscapedByte(0xff - (XBeeTxChecksum & 0xff));
}

void SendFidoMessage(psMessage_t *msg) {
    XBeeTxPacket.packetHeader.apiIdentifier = TRANSMIT_16;
    XBeeTxPacket.packetHeader.frameId = XBeeTxSequenceNumber;
    XBeeTxPacket.packetHeader.destinationMSB = 0;
    XBeeTxPacket.packetHeader.destinationLSB = PROXY_ADDRESS;
    XBeeTxPacket.packetHeader.options = 0;

    memcpy(&XBeeTxPacket.message, msg, sizeof (psMessage_t));

    SendApiPacket((uint8_t*) & XBeeTxPacket, (msg->header.length + sizeof (XBeeTxPacket.packetHeader) + SIZEOF_HEADER));
}

//Tx thread - sends messages to the XBee

static void XBeeTxTask(void *pvParameters) {
    psMessage_t msg;

    LogRoutine("TX ready");

    for (;;) {

        //wait for a message
        if (xQueueReceive(xbeeTxQueue, &msg, portMAX_DELAY) == pdTRUE) {

            xSemaphoreTake(XBeeTxMutex, 5000);

            SendFidoMessage(&msg);

            LogRoutine("XBee TX: %s", psLongMsgNames[msg.header.messageType]);

            if (xSemaphoreTake(XBeeTxResponseSemaphore, 2000) == pdTRUE) {
                if (latestTxStatus.status != 0) {
                    XBeeSendErrors++;
                    SetCondition(MCP_XBEE_COMMS_ERRORS);
                    DebugPrint("XBee Tx Status : %s", txStatusNames[latestTxStatus.status]);
                } else {
                    DebugPrint("XBee Tx Status OK");
                }
            } else {
                XBeeSendErrors++;
                SetCondition(MCP_XBEE_COMMS_ERRORS);
                DebugPrint("XBee Tx Timeout");
            }

            xSemaphoreGive(XBeeTxMutex);

            if (msg.header.messageType == SYSLOG_MSG) {
                switch (msg.logPayload.severity) {
                    case SYSLOG_ROUTINE:
                        XBeeRoutine--;
                        break;
                    case SYSLOG_INFO:
                        XBeeInfo--;
                        break;
                    case SYSLOG_WARNING:
                        XBeeWarning--;
                        break;
                    case SYSLOG_ERROR:
                        XBeeError--;
                    default:
                        break;
                }
            }
        }
    }
    return;
}

int ReadEscapedData(uint8_t *pkt, int len) {
    uint8_t *next = pkt;
    int count = len;
    uint8_t readByte;

    do {
        UART_DATA dat = Serial_read(XBEE_UART);
        readByte = dat.data8bit;

        if (readByte == ESCAPEE) {

            dat = Serial_read(XBEE_UART);
            readByte = dat.data8bit;

            *next = (readByte ^ CHAR_XOR);
        } else {
            *next = readByte;
        }

        XBeeRxChecksum += *next;

        next++;
        count--;
    } while (count > 0);

    return 0;
}

int ReadApiPacket() {
    int reply;
    uint8_t checksum;

    UART_DATA dat;
    uint8_t lengthBytes[2];

    do {
        //read until frame delimiter
        dat = Serial_read(XBEE_UART);
    } while (dat.data8bit != FRAME_DELIMITER);

    reply = ReadEscapedData(lengthBytes, 2); //read length

    int length = lengthBytes[1];
    if (length > sizeof (XBeeRxPacket)) length = sizeof (XBeeRxPacket);

    XBeeRxChecksum = 0; //length not included

    reply += ReadEscapedData((uint8_t*) & XBeeRxPacket, length);
    reply += ReadEscapedData((uint8_t*) & checksum, 1);

    if (reply < 0 || (XBeeRxChecksum & 0xff) != 0xff) return -1;
    else return 0;
}

//receives messages and passes to broker

static void XBeeRxTask(void *pvParameters) {
    psMessage_t *msg;

    LogRoutine("XBee RX ready");

    for (;;) {
        if (ReadApiPacket() == 0) {
            switch (XBeeRxPacket.apiIdentifier) {
                case MODEM_STATUS:
                    latestXBeeModemStatus = XBeeRxPacket.modemStatus.status;
                    DebugPrint("XBee status : %i", latestXBeeModemStatus);
                    break;
                case TRANSMIT_STATUS:
                    latestTxStatus.status = XBeeRxPacket.txStatus.status;
                    latestTxStatus.frameId = XBeeRxPacket.txStatus.frameId;
                    //				DebugPrint("Tx Status: %i", latestTxStatus.status);
                    xSemaphoreGive(XBeeTxResponseSemaphore);
                    break;
                case AT_RESPONSE:
                    DebugPrint("AT Response packet");
                    memcpy(&XBeeATResponse, &XBeeRxPacket, sizeof (XBeeATResponse));
                    xSemaphoreGive(XBeeATResponseSemaphore);
                    break;
                case RECEIVE_16:
                    msg = &XBeeRxPacket.fidoMessage.message;

                    if (msg->header.messageType != SYSLOG_MSG //don't log log messages!!
                            && msg->header.messageType != TICK_1S) //& don't log ticks
                    {
                        LogRoutine("XBee RX: %s", psLongMsgNames[msg->header.messageType]);
                    }


                    if (msg->header.messageType == KEEPALIVE) {
                        xQueueSendToBack(xbeeTxQueue, msg, 0);
                    } else {
                        if ((msg->header.source != XBEE) && (msg->header.source != APP_XBEE) && (msg->header.source != APP_OVM) && (msg->header.source != ROBO_APP)) {
                            DebugPrint("XBee Ignored: %s", psLongMsgNames[msg->header.messageType]);
                            XBeeAddressIgnored++;
                        } else {
                            DebugPrint("XBee RX: %s", psLongMsgNames[msg->header.messageType]);
                            //route the message
                            psForwardMessage(msg, 0);
                            XBeeMessagesReceived++;

                            xbeeOnline = true;
                            APPonline = true;
                            xTimerReset(xbeeTimeout, 0);
                        }
                    }
                    break;
                case RECEIVE_IO_16:
                    //tbd

                    break;
                default:
                    //ignore
                    break;
            }
        } else {
            DebugPrint("XBee Rx Error");
            SetCondition(MCP_XBEE_COMMS_ERRORS);
        }
    }
    return;
}

//Use prior to entering API mode and creating threads
#define MAX_REPLY 10

int SendATCommand(char *cmdString, char *replyString) {
    int reply;
    size_t len = strlen(cmdString);
    char *next = cmdString;

    while (len-- > 0) {
        XBEE_write(*next++);
    }

    next = replyString;
    len = MAX_REPLY - 1;

    do {
        UART_DATA dat = Serial_read(XBEE_UART);
        *next = dat.data8bit;
    } while (len-- > 0 && *next++ != '\r');

    *next = '\0';

    if (reply < 0) return -1;
    else return 0;
}

int SetXBeeRegister(const char *atCommand, uint8_t value) {
    int reply;
    char cmdString[MAX_REPLY];
    char replyString[MAX_REPLY];

    DebugPrint("XBee Set %s = %i", atCommand, value);

    sprintf(cmdString, "AT%s%i\r", atCommand, value);

    reply = SendATCommand(cmdString, replyString);

    if (reply < 0 || strncmp(replyString, "OK", 2) != 0)
        return -1;
    else return 0;
}

int GetXBeeRegister(const char *atCommand) {
    int reply;
    char cmdString[MAX_REPLY];
    char replyString[MAX_REPLY];

    DebugPrint("XBee Get %s", atCommand);

    sprintf(cmdString, "AT%s\r", atCommand);

    reply = SendATCommand(cmdString, replyString);

    if (reply >= 0) {
        reply = -1;
        sscanf(replyString, "%i", &reply);
        return reply;
    } else return -1;
}

//Get/Set 1 byte registers using API

int APISetXBeeRegister1(const char *atCommand, uint8_t value) {
    uint8_t response;

    LogRoutine("XBee Set %s = %i", atCommand, value);

    xSemaphoreTake(XBeeTxMutex, 1000);

    XBeeATPacket.packetHeader.apiIdentifier = AT_COMMAND;
    XBeeATPacket.packetHeader.frameId = 0;
    XBeeATPacket.packetHeader.ATcommand[0] = atCommand[0];
    XBeeATPacket.packetHeader.ATcommand[1] = atCommand[1];

    XBeeATPacket.byteData = value;

    SendApiPacket((uint8_t*) & XBeeATPacket, sizeof (XBeeATPacket.packetHeader) + 1);

    if (xSemaphoreTake(XBeeATResponseSemaphore, 500) == pdTRUE) {
        response = XBeeATResponse.status;
    } else {
        response = -1;
    }
    xSemaphoreGive(XBeeTxMutex);

    return (response == 0 ? 0 : -1);
}

int APIGetXBeeRegister1(const char *atCommand) {
    int response;

    LogRoutine("XBee Get %s", atCommand);

    xSemaphoreTake(XBeeTxMutex, 1000);

    XBeeATPacket.packetHeader.apiIdentifier = AT_COMMAND;
    XBeeATPacket.packetHeader.frameId = 0;
    XBeeATPacket.packetHeader.ATcommand[0] = atCommand[0];
    XBeeATPacket.packetHeader.ATcommand[1] = atCommand[1];

    SendApiPacket((uint8_t*) & XBeeATPacket, sizeof (XBeeATPacket.packetHeader));

    if (xSemaphoreTake(XBeeATResponseSemaphore, 500) == pdTRUE) {
        if (XBeeATResponse.status == 0) response = XBeeATResponse.byteData;
        else response = -1;
    } else {
        LogError("GetXBReg timeout");
        response = -1;
    }
    xSemaphoreGive(XBeeTxMutex);

    return response;
}
//tx power level

int SetPowerLevel(int pl) {
    return SetXBeeRegister(POWER_LEVEL, pl);
}

int GetPowerLevel() {
    return GetXBeeRegister(POWER_LEVEL);
}
//get into API mode

int EnterCommandMode() {
    char replyString[MAX_REPLY];
    vTaskDelay(2000);
    XBEE_write('+');
    XBEE_write('+');
    XBEE_write('+');
    vTaskDelay(2000);
    //check for OK
    int reply = SendATCommand("", replyString);

    DebugPrint("Enter Command Mode: %s", replyString)

    if (reply < 0 || strncmp(replyString, "OK", 2) != 0)
        return -1;
    else return 0;
}

int EnterAPIMode() {
    return SetXBeeRegister("AP", 2);
}

void XBeeSendStats() {
    psMessage_t msg;
    psInitPublish(msg, COMMS_STATS);

    strncpy(msg.commsStatsPayload.destination, "BEE", 4);

    msg.commsStatsPayload.messagesSent = XBeeMessagesSent;
    msg.commsStatsPayload.addressDiscarded = XBeeAddressDiscarded;
    msg.commsStatsPayload.congestionDiscarded = XBeeCongestionDiscarded; //congestion
    msg.commsStatsPayload.logMessagesDiscarded = XBeeLogMessagesDiscarded;
    msg.commsStatsPayload.sendErrors = XBeeSendErrors;
    msg.commsStatsPayload.messagesReceived = XBeeMessagesReceived;
    msg.commsStatsPayload.addressIgnored = XBeeAddressIgnored;
    msg.commsStatsPayload.receiveErrors = XBeeReceiveErrors; //wrong address
    msg.commsStatsPayload.parseErrors = XBeeParseErrors;

    psForwardMessage(&msg, 0);
}

void xbeeOfflineTimerCallback(TimerHandle_t xTimer) {
    xbeeOnline = false;
}