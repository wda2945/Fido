/*
 * File:   serialbroker.c
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
#include "pubsubdata.h"
#include "pubsubparser.h"

#include "syslog/syslog.h"
#include "pubsub/notifications.h"
#include "softwareprofile.h"
#include "brokerq.h"
#include "broker_debug.h"


//Serial Tx queue
BrokerQueue_t uartTxQueue = {NULL, NULL,
		PTHREAD_MUTEX_INITIALIZER,
		PTHREAD_COND_INITIALIZER
};

//RX and TX UART threads
void *RxThread(void *a);
void *TxThread(void *a);

#define MAX_UART_MESSAGE (sizeof(psMessage_t) + 10)

int picUartFD;

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
int parseErrors;
int receiveErrors;

int SerialBrokerInit()
{
	struct termios settings;

//	if (load_device_tree(PS_UART_OVERLAY) < 0)
//	{
//		ERRORPRINT("broker uart overlay: %s failed\n", PS_UART_OVERLAY);
//		return -1;
//	}
//	else {
//		DEBUGPRINT("broker uart overlay OK\n", PS_UART_DEVICE);
//	}

	if (uart_setup(PS_TX_PIN, PS_RX_PIN) < 0)
	{
		ERRORPRINT("uart pinmux fail");
		return -1;
	}
	else {
		DEBUGPRINT("broker uart pinmux OK\n");
	}

	sleep(1);

	//initialize UART
	picUartFD = open(PS_UART_DEVICE, O_RDWR | O_NOCTTY);
	int retryCount = 10;

	while (picUartFD < 0 && retryCount-- > 0) {
		ERRORPRINT("Open %s: %s\n",PS_UART_DEVICE, strerror(errno));
		sleep(1);
		picUartFD = open(PS_UART_DEVICE, O_RDWR | O_NOCTTY);
	}

	if (picUartFD < 0) {
		return -1;
	}
	else {
		DEBUGPRINT("%s opened\n", PS_UART_DEVICE);
	}


	if (tcgetattr(picUartFD, &settings) != 0) {
		ERRORPRINT("tcgetattr: %s\n", strerror(errno));
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

	if (tcsetattr(picUartFD, TCSANOW, &settings) != 0) {
		ERRORPRINT("tcsetattr: %s\n", strerror(errno));
		return -1;
	}

	DEBUGPRINT("uart configured\n");

	//start RX & TX threads
	pthread_t thread;

	int s = pthread_create(&thread, NULL, RxThread, NULL);
	if (s != 0)
	{
		ERRORPRINT("Rx Thread: %s\n", strerror(s));
		return -1;
	}
	s = pthread_create(&thread, NULL, TxThread, NULL);
	if (s != 0)
	{
		ERRORPRINT("Tx Thread: %s\n", strerror(s));
		return -1;
	}

	return 0;
}

//called by the broker to see whether a message should be queued for the TX Thread

void SerialBrokerProcessMessage(psMessage_t *msg)
{
	if ((msg->header.source != OVERMIND ) && (msg->header.source != APP_OVM)) return;	//don't echo messages

//	DEBUGPRINT("Serial: %s\n", psLongMsgNames[msg->header.messageType]);

	//check for log messages to send to MCP
    if (msg->header.messageType == SYSLOG_MSG) {
        //count queued log messages and limit
        switch (msg->logPayload.severity) {
            case SYSLOG_ROUTINE:
                if (uartRoutine > (int)maxUARTRoutine) {
                    logMessagesDiscarded++;
                    return;
                }
                ++uartRoutine;
                break;
            case SYSLOG_INFO:
                if (uartInfo > (int)maxUARTInfo) {
                    logMessagesDiscarded++;
                    return;
                }
                ++uartInfo;
                break;
            case SYSLOG_WARNING:
                if (uartWarning > (int)maxUARTWarning) {
                    logMessagesDiscarded++;
                    return;
                }
                ++uartWarning;
                break;
            case SYSLOG_ERROR:
                if (uartError > (int)maxUARTError) {
                    logMessagesDiscarded++;
                    return;
                }
                ++uartError;
            default:
                break;
        }
    }

	//add to transmit queue
	CopyMessageToQ(&uartTxQueue, msg);

//	DEBUGPRINT("uart: Queuing for send: %s\n", psLongMsgNames[msg->header.messageType]);

}

//Tx thread - sends messages from the Tx Q
void *TxThread(void *a) {
	int i;
	int length;
	char messageBuffer[MAX_UART_MESSAGE];
	char *current;
	long written;
	int checksum;
	unsigned char sequenceNumber = 0;

	DEBUGPRINT("TX ready\n");

	for (;;) {

		//wait for a message
		psMessage_t *msg = GetNextMessage(&uartTxQueue);

		length =  msg->header.length + 8;   //stx, header(5), payload, checksum

		current = messageBuffer;

		*current++ = STX_CHAR;
		*current++ = msg->header.length;
		*current++ = ~msg->header.length;
		*current++ = sequenceNumber;
		sequenceNumber = (sequenceNumber + 1) & 0xff;
		*current++ = msg->header.source;
		*current++ = msg->header.messageType;
		*current++ = msg->header.messageTopic;

		memcpy(current, msg->packet, msg->header.length);

		checksum = 0;
		for (i = 1; i < length-1; i++) {
			checksum += messageBuffer[i];
		}
		messageBuffer[length-1] = (checksum & 0xff);

		written = write(picUartFD, messageBuffer, length);

		if (written == length)
		{
			DEBUGPRINT("uart TX: %s\n", psLongMsgNames[msg->header.messageType]);
		} else {
			ERRORPRINT("uart TX:. %s\n", strerror(errno));
			SetCondition(OVM_MCP_COMMS_ERRORS);
		}

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

static char *parseErrorsText[] = PARSE_RESULTS;
//receives messages and passes to broker
void *RxThread(void *a) {

	ParseResult_enum result;

	psMessage_t msg;
	uint8_t c;

	status_t parseStatus;

	parseStatus.noCRC 		= 0;	///< Do not expect a CRC, if > 0
	parseStatus.noSeq		= 0;	///< Do not check seq #s, if > 0
	parseStatus.noLength2	= 0;	///< Do not check for duplicate length, if > 0
	parseStatus.noTopic		= 0;	///< Do not check for topic ID, if > 0
	ResetParseStatus(&parseStatus);

	DEBUGPRINT("RX ready\n");

	for (;;) {
		do {

			int count = (int) read(picUartFD, &c, 1);

			if (count == 1)
			{
				result = ParseNextCharacter(c, &msg, &parseStatus);

				switch (result) {
				case PARSE_OK:
				case PARSED_MESSAGE:
					break;
				case PARSE_SEQUENCE_ERROR:
					if (messagesReceived > 1)	//ignore first sequence error
					{
						receiveErrors++;
						ERRORPRINT("Parse: %s\n", parseErrorsText[result]);
						SetCondition(OVM_MCP_COMMS_ERRORS);
					}
					result = PARSE_OK;
					break;

				default:
					//error
					ERRORPRINT("Parse: %s\n", parseErrorsText[result]);
					parseErrors++;
					SetCondition(OVM_MCP_COMMS_ERRORS);
					result = PARSE_OK;
					break;
				}
			}
			else if (count < 0)
			{
				ERRORPRINT("uart RX: %s\n", strerror(errno));
				SetCondition(OVM_MCP_COMMS_ERRORS);
				result = PARSE_OK;
			}

		} while (result == PARSE_OK);


		if (msg.header.source != OVERMIND) {
			if (msg.header.messageType != TICK_1S)
			{
				DEBUGPRINT("uart RX: %s\n", psLongMsgNames[msg.header.messageType]);
			}
			//route the message
			NewBrokerMessage(&msg);
			messagesReceived++;
		}
		else
		{
			addressIgnored++;
		}
	}
	return 0;
}

void SendStats() {
    psMessage_t msg;
    psInitPublish(msg, COMMS_STATS);

    strncpy(msg.commsStatsPayload.destination, "MCP", 4);
    msg.commsStatsPayload.messagesSent 			= messagesSent;
    msg.commsStatsPayload.sendErrors 			= sendErrors;
    msg.commsStatsPayload.addressDiscarded 		= addressDiscarded;
    msg.commsStatsPayload.congestionDiscarded 	= congestionDiscarded; //congestion
    msg.commsStatsPayload.logMessagesDiscarded 	= logMessagesDiscarded;
    msg.commsStatsPayload.messagesReceived 		= messagesReceived;
    msg.commsStatsPayload.addressIgnored 		= addressIgnored; //wrong address
    msg.commsStatsPayload.parseErrors 			= parseErrors; //wrong address
    msg.commsStatsPayload.receiveErrors 		= receiveErrors;

    NewBrokerMessage(&msg);
}

