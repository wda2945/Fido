/*
 * File:   xbee serialbroker.c
 * Author: martin
 *
 * Created on November 22, 2013
 */

//Forwards PubSub messages via UART to PIC

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include "uart.h"
#include "common.h"
#include "gpio.h"
#include "PubSubData.h"
#include "PubSubParser.h"

#include "syslog/syslog.h"
#include "pubsub/notifications.h"
#include "SoftwareProfile.h"
#include "brokerQ.h"
#include "broker_debug.h"
#include "xbee.h"
#include "agent/agent.h"

//Message Tx queue
BrokerQueue_t xbeeTxQueue = {NULL, NULL,
		PTHREAD_MUTEX_INITIALIZER,
		PTHREAD_COND_INITIALIZER
};

//RX and TX UART threads
void *RxThread(void *a);
void *TxThread(void *a);

typedef struct {
	struct {
		uint8_t apiIdentifier;
		uint8_t frameId;
		uint8_t destinationMSB;
		uint8_t destinationLSB;
		uint8_t options;
	} packetHeader;
	psMessage_t message;
} TxPacket_t;				//regular Fido data packet (Tx)

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
		int 	intData;
	};
} ATPacket_t;				//XBee AT command packet

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
} RxPacket_t;					//regular Fido data packet (Rx) + status packets

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
		int 	intData;
	};
} ATResponse_t;					//XBee AT Command Response

struct {
	XBeeTxStatus_enum 	status;
	uint8_t 			frameId;
} latestTxStatus;
pthread_cond_t txStatusCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t txStatusMutex = PTHREAD_MUTEX_INITIALIZER;

XBeeModemStatus_enum latestModemStatus;

TxPacket_t txPacket;		//regular Fido data packet (Tx)
ATPacket_t atPacket;		//XBee AT command packet

RxPacket_t rxPacket;		//regular Fido data packet (Rx) + status packets
ATResponse_t atResponse;	//XBee AT Command Response

pthread_cond_t atResponseCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t atResponseMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t txMutex = PTHREAD_MUTEX_INITIALIZER;

int xbeeUartFD;
int txChecksum;
int rxChecksum;
uint8_t txSequenceNumber = 1;
uint8_t rxSequenceNumber;

//counts for throttling logging messages
int uartRoutine = 0;
int uartInfo = 0;
int uartWarning = 0;
int uartError = 0;

//statistics collection
//TX
int messagesSent;
int addressDiscarded;
int congestionDiscarded;      //congestion
int logMessagesDiscarded;
int sendErrors;
//RX
int messagesReceived;
int addressIgnored;        //wrong address
int receiveErrors;
int parseErrors;

char *txStatusNames[] = TX_STATUS_NAMES;

int SetRegister(const char *atCommand, uint8_t value);
int GetRegister(const char *atCommand);

int EnterCommandMode();
int EnterAPIMode();

int XBeeBrokerInit()
{
	struct termios settings;

//	gpio_export(XBEE_RESET_GPIO);
//	gpio_set_direction(XBEE_RESET_GPIO, 1);	//input

	gpio_export(XBEE_DTR_GPIO);
	gpio_set_direction(XBEE_DTR_GPIO, 0);	//output
	gpio_set_value(XBEE_DTR_GPIO, 0);		//active low

	gpio_export(XBEE_STATUS_GPIO);
	gpio_set_direction(XBEE_STATUS_GPIO, 1);	//input

	gpio_export(XBEE_ASSOCIATE_GPIO);
	gpio_set_direction(XBEE_ASSOCIATE_GPIO, 1);	//input

	gpio_export(XBEE_RESET_OUT_GPIO);
	gpio_set_direction(XBEE_RESET_OUT_GPIO, 0);	//output
	gpio_set_value(XBEE_RESET_OUT_GPIO, 1);		//reset XBee
	sleep(1);
	gpio_set_value(XBEE_RESET_OUT_GPIO, 0);		//reset XBee

	if (uart_setup(PS_TX_PIN, PS_RX_PIN) < 0)
	{
		LogError("uart pinmux\n");
		return -1;
	}

	//initialize UART
	xbeeUartFD = open(PS_UART_DEVICE, O_RDWR | O_NOCTTY);

	if (xbeeUartFD < 0) {
		LogError("Open %s: %s",PS_UART_DEVICE, strerror(errno));
		return -1;
	}
	else
	{
		DEBUGPRINT("%s opened\n", PS_UART_DEVICE);
	}

	if (tcgetattr(xbeeUartFD, &settings) != 0) {
		LogError("tcgetattr: %s\n", strerror(errno));
		return -1;
	}

	//no processing
	settings.c_iflag = 0;
	settings.c_oflag = 0;
	settings.c_lflag = 0;
	settings.c_cflag = CLOCAL | CREAD | CS8;        //no modem, 8-bits

	//baudrate
	cfsetospeed(&settings, PS_UART_BAUDRATE);
	cfsetispeed(&settings, PS_UART_BAUDRATE);

	if (tcsetattr(xbeeUartFD, TCSANOW, &settings) != 0) {
		LogError("tcsetattr: %s\n", strerror(errno));
		return -1;
	}

	DEBUGPRINT("xbee uart configured\n");

	bool xbeeReady = false;
	int i;

	for (i=0; i<10; i++)
	{
		if (EnterCommandMode() != 0) continue;

		int power = GetPowerLevel();

		if (power < 0)
		{
			LogError("Read PowerLevel fail\n");
			continue;
		}

		DEBUGPRINT("xbee power level %i\n", power);

		if (EnterAPIMode() != 0) continue;

		xbeeReady = true;
		break;
	}
	if (xbeeReady)
	{
		DEBUGPRINT("xbee ready\n");

		//start RX & TX threads
		pthread_t thread;

		int s = pthread_create(&thread, NULL, RxThread, NULL);
		if (s != 0)
		{
			LogError("Rx Thread: %s\n", strerror(s));
			return -1;
		}
		s = pthread_create(&thread, NULL, TxThread, NULL);
		if (s != 0)
		{
			LogError("Tx Thread: %s\n", strerror(s));
			return -1;
		}
		return 0;
	}
	else return -1;
}

//called by the broker to see whether a message should be queued for the TX Thread

void XBeeBrokerProcessMessage(psMessage_t *msg)
{
	if ((msg->header.source == XBEE) || (msg->header.source == APP_XBEE)) 	//don't echo messages
	{
		//add to transmit queue
		CopyMessageToQ(&xbeeTxQueue, msg);
	}
}

int WriteByte(uint8_t _b)
{
	uint8_t b = _b;
	ssize_t written;

	do {
		written = write(xbeeUartFD, &b, 1);
	} while (written == 0 || written == EAGAIN);

	return (written == 1 ? 0: -1);
}

int WriteEscapedByte(uint8_t b)
{
	int reply;
	txChecksum += b;

	switch (b)
	{
	case FRAME_DELIMITER:
	case ESCAPE:
	case XON:
	case XOFF:
		reply = WriteByte(ESCAPEE);
		reply += WriteByte(b ^ CHAR_XOR);
		break;
	default:
		reply = WriteByte(b);
		break;
	}
	return (reply < 0 ? -1 : 0);
}

int SendApiPacket(uint8_t *pkt, int len)
{
	int reply;
	uint8_t *next = pkt;
	int count = len;

	reply = WriteByte(FRAME_DELIMITER);
	reply += WriteEscapedByte((len >> 8) & 0xff);
	reply += WriteEscapedByte(len & 0xff);

	txChecksum = 0;

	do {
		reply += WriteEscapedByte(*next++);
		count--;
	} while (count > 0);

	reply += WriteEscapedByte(0xff - (txChecksum & 0xff));

	return (reply < 0 ? -1 : 0);
}

int SendFidoMessage(psMessage_t *msg)
{
	if (++txSequenceNumber > 0xf0) txSequenceNumber = 1;

	txPacket.packetHeader.apiIdentifier 	= TRANSMIT_16;
	txPacket.packetHeader.frameId			= txSequenceNumber;
	txPacket.packetHeader.destinationMSB	= 0;
	txPacket.packetHeader.destinationLSB	= MCP_ADDRESS;
	txPacket.packetHeader.options			= 0;

	memcpy(&txPacket.message, msg, sizeof(psMessage_t));

	return SendApiPacket((uint8_t*)&txPacket, (msg->header.length + sizeof(txPacket.packetHeader) + SIZEOF_HEADER));
}

//Tx thread - sends messages from the Tx Q
void *TxThread(void *a) {
    
	DEBUGPRINT("XBee TX ready\n");

	for (;;) {
		struct timeval now;
		struct timespec wait_time;

		//wait for a message
		psMessage_t *msg = GetNextMessage(&xbeeTxQueue);

		pthread_mutex_lock(&txMutex);

		pthread_mutex_lock(&txStatusMutex);

		if (SendFidoMessage(msg) == 0)
		{
			DEBUGPRINT("XBee TX: %s\n", psLongMsgNames[msg->header.messageType]);

			gettimeofday(&now, NULL);
			wait_time.tv_sec = now.tv_sec + 5;
			wait_time.tv_nsec = now.tv_usec * 1000;
			latestTxStatus.frameId = txSequenceNumber;
			latestTxStatus.status = XBEE_TX_TIMEOUT;

			pthread_cond_timedwait(&txStatusCond, &txStatusMutex, &wait_time);

			if (latestTxStatus.frameId != txSequenceNumber || latestTxStatus.status != XBEE_TX_SUCCESS)
			{
				ERRORPRINT("XBee TX %i Status: %s\n", txSequenceNumber, txStatusNames[latestTxStatus.status]);
				SetCondition(XBEE_MCP_COMMS_ERRORS);
			}
		} else {
			ERRORPRINT("XBee SendFidoMessage() error\n ");
			SetCondition(XBEE_MCP_COMMS_ERRORS);
		}

		pthread_mutex_unlock(&txStatusMutex);

		pthread_mutex_unlock(&txMutex);

		if (psDefaultTopics[msg->header.messageType] == LOG_TOPIC)
		{
			switch (msg->logPayload.severity)
			{
			case SYSLOG_ROUTINE:
				uartRoutine--;
				break;
			case SYSLOG_INFO:
				uartInfo--;
				break;
			case SYSLOG_WARNING:
				uartWarning--;
				break;
			case SYSLOG_ERROR:
				uartError--;
			default:
				break;
			}
		}
		DoneWithMessage(msg);
	}
	return 0;
}

int ReadEscapedData(uint8_t *pkt, int len)
{
	uint8_t *next = pkt;
	int count = len;
	uint8_t readByte;
	ssize_t reply;

	do {
		do {
			reply = read(xbeeUartFD, &readByte, 1);
		} while (reply == 0 || reply == EAGAIN);

		if (reply < 0) return -1;

		if (readByte == ESCAPEE)
		{
			do {
				reply = read(xbeeUartFD, &readByte, 1);
			} while (reply == 0 || reply == EAGAIN);

			if (reply < 0) return -1;

			*next = (readByte ^ CHAR_XOR);
		}
		else
		{
			*next = readByte;
		}

		rxChecksum += *next;

		next++;
		count--;
	} while (count > 0);
	return 0;
}

int ReadApiPacket()
{
	ssize_t reply;
	uint8_t delimiter = 0;
	struct {
		uint8_t msb;
		uint8_t lsb;
	} pktLength;
	uint8_t checksum;

	do {
		reply = read(xbeeUartFD, &delimiter, 1);
	} while (reply == 0 || reply == EAGAIN || (reply == 1 && delimiter != FRAME_DELIMITER));

	if (reply < 0) return -1;

	reply = ReadEscapedData((uint8_t*) &pktLength, 2);

	if (reply < 0) return -1;

	int length = pktLength.lsb;
	if (length > sizeof(rxPacket)) length = sizeof(rxPacket);

	rxChecksum = 0;

	reply += ReadEscapedData((uint8_t*)&rxPacket, length);
	reply += ReadEscapedData(&checksum, 1);

	if (reply < 0 || (rxChecksum & 0xff) != 0xff)
		return -1;
	else
		return 0;
}

//receives messages and passes to broker
void *RxThread(void *a) {
	psMessage_t *msg;

	DEBUGPRINT("XBee RX ready\n");

	for (;;) {
		if (ReadApiPacket() == 0)
		{
			switch(rxPacket.apiIdentifier)
			{
			case MODEM_STATUS:
				latestModemStatus = rxPacket.modemStatus.status;
				LogInfo("XBee status : %i", rxPacket.modemStatus.status);
				break;
			case TRANSMIT_STATUS:
				DEBUGPRINT("Tx Status: %i\n", rxPacket.txStatus.status);
				pthread_mutex_lock(&txStatusMutex);

				latestTxStatus.status = rxPacket.txStatus.status;
				latestTxStatus.frameId = rxPacket.txStatus.frameId;

				pthread_mutex_unlock(&txStatusMutex);
				pthread_cond_signal(&txStatusCond);
				break;
			case AT_RESPONSE:
				DEBUGPRINT("AT Response packet\n");
				pthread_mutex_lock(&atResponseMutex);

				memcpy(&atResponse, &rxPacket, sizeof(atResponse));

				pthread_mutex_unlock(&atResponseMutex);
				pthread_cond_signal(&atResponseCond);
				break;
			case RECEIVE_16:
				msg = &rxPacket.fidoMessage.message;

				if (msg->header.messageType == KEEPALIVE)
				{
					msg->header.source = 0;
					AgentProcessMessage(msg);
				}
				else
				{
					if ((msg->header.source != XBEE) && (msg->header.source != APP_XBEE) && (msg->header.source != APP_OVM) && (msg->header.source != ROBO_APP))
					{
						DEBUGPRINT("XBee RX: %s\n", psLongMsgNames[msg->header.messageType]);
						//route the message
						NewBrokerMessage(msg);
						messagesReceived++;
					}
					else
					{
						DEBUGPRINT("XBee Ignored: %s\n", psLongMsgNames[msg->header.messageType]);
						addressIgnored++;
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
		}
		else
		{
			ERRORPRINT("XBee ReadApiPacket() Error");
			SetCondition(XBEE_MCP_COMMS_ERRORS);
		}
	}
	return 0;
}

//Get/Set 1 byte registers
//Use prior to creating threads
#define MAX_REPLY 10
int SendATCommand(char *cmdString, char *replyString)
{
	ssize_t reply;
	size_t len = strlen(cmdString);
	char *next = cmdString;

	while (len-- > 0)
	{
		WriteByte(*next++);
	}

	next = replyString;
	len = MAX_REPLY-1;

	do {
		do {
			reply = read(xbeeUartFD, next, 1);
		} while (reply == 0 || reply == EAGAIN);
	} while (len-- > 0 && *next++ != '\r');

	*next = '\0';

	if (reply < 0) return -1;
	else return 0;
}
int SetRegister(const char *atCommand, uint8_t value)
{
	int reply;
	char cmdString[MAX_REPLY];
	char replyString[MAX_REPLY];

	DEBUGPRINT("XBee Set %s = %i\n", atCommand, value);

	sprintf(cmdString, "AT%s%i\r", atCommand, value);

	reply = SendATCommand(cmdString, replyString);

	if (reply < 0 || strncmp(replyString, "OK", 2) != 0)
		return -1;
	else return 0;
}
int GetRegister(const char *atCommand)
{
	int reply;
	char cmdString[MAX_REPLY];
	char replyString[MAX_REPLY];

	DEBUGPRINT("XBee Get %s\n", atCommand);

	sprintf(cmdString, "AT%s\r", atCommand);

	reply = SendATCommand(cmdString, replyString);

	if (reply >= 0)
	{
		reply = -1;
		sscanf(replyString, "%i", &reply);
		return reply;
	}
	else return -1;
}

int APISetRegister1(const char *atCommand, uint8_t value)
{
	uint8_t response;

	DEBUGPRINT("XBee Set %s = %i\n", atCommand, value);

	pthread_mutex_lock(&txMutex);

	atPacket.packetHeader.apiIdentifier 	= AT_COMMAND;
	atPacket.packetHeader.frameId			= 0;
	atPacket.packetHeader.ATcommand[0] = atCommand[0];
	atPacket.packetHeader.ATcommand[1] = atCommand[1];

	atPacket.byteData = value;

	SendApiPacket((uint8_t*)&atPacket, sizeof(atPacket.packetHeader) + 1);

	pthread_cond_wait(&atResponseCond, &atResponseMutex);

	if (atResponse.status == 0) response = 0;
	else response = -1;

	pthread_mutex_unlock(&atResponseMutex);

	pthread_mutex_unlock(&txMutex);

	return (response == 0 ? 0 : -1);
}

int APIGetRegister1(const char *atCommand)
{
	int response;

	DEBUGPRINT("XBee Get %s\n", atCommand);

	pthread_mutex_lock(&txMutex);

	atPacket.packetHeader.apiIdentifier 	= AT_COMMAND;
	atPacket.packetHeader.frameId			= 0;
	atPacket.packetHeader.ATcommand[0] = atCommand[0];
	atPacket.packetHeader.ATcommand[1] = atCommand[1];

	SendApiPacket((uint8_t*)&atPacket, sizeof(atPacket.packetHeader));

	pthread_cond_wait(&atResponseCond, &atResponseMutex);

	if (atResponse.status == 0) response = atResponse.byteData;
	else response = -1;

	pthread_mutex_unlock(&atResponseMutex);

	pthread_mutex_unlock(&txMutex);

	return response;
}
//tx power level
int SetPowerLevel(int pl)
{
	return SetRegister(POWER_LEVEL, pl);
}
int GetPowerLevel()
{
	return GetRegister(POWER_LEVEL);
}

int EnterCommandMode()
{
	int reply;
	char replyString[MAX_REPLY];
	sleep(1);
	WriteByte('+');
	WriteByte('+');
	WriteByte('+');
	sleep(1);

	reply = SendATCommand("", replyString);

	DEBUGPRINT("Command Mode: %s\n", replyString);

	if (reply < 0 || strncmp(replyString, "OK", 2) != 0)
		return -1;
	else return 0;
}
int EnterAPIMode()
{
	return SetRegister("AP", 2);
}

void SendStats() {
    psMessage_t msg;
    psInitPublish(msg, COMMS_STATS);

    strncpy(msg.commsStatsPayload.destination, "MCP", 4);
    msg.commsStatsPayload.messagesSent = messagesSent;
    msg.commsStatsPayload.sendErrors	= sendErrors;
    msg.commsStatsPayload.addressDiscarded = addressDiscarded;
    msg.commsStatsPayload.congestionDiscarded = congestionDiscarded; //congestion
    msg.commsStatsPayload.messagesReceived = messagesReceived;
    msg.commsStatsPayload.addressIgnored = addressIgnored; //wrong address
    msg.commsStatsPayload.receiveErrors = receiveErrors;
    msg.commsStatsPayload.parseErrors = parseErrors;

    NewBrokerMessage(&msg);
}

