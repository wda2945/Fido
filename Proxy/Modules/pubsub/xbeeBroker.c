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
#include "pubsubdata.h"
#include "pubsubparser.h"

#include "syslog/syslog.h"
#include "pubsub/notifications.h"
#include "softwareprofile.h"
#include "brokerq.h"
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
	union {						//up to 4 bytes of parameter
		struct {
			uint8_t byteData;
			uint8_t fill[3];
		};
		struct {
			uint16_t uint16Data;
			uint16_t fill16;
		};
		int 	intData;
	};
} ATPacket_t;				//XBee Local AT command packet

typedef struct {
	struct {
		uint8_t apiIdentifier;	//0x17
		uint8_t frameId;
		uint8_t address64bit[8];
		uint8_t destinationMSB;
		uint8_t destinationLSB;
		uint8_t	commandOptions;	//0x02
		uint8_t ATcommand[2];
	} packetHeader;
	union {
		struct {
			uint8_t byteData;
			uint8_t fill[3];
		};
		struct {
			uint16_t uint16Data;
			uint16_t fill16;
		};
		int 	intData;
	};
} RemoteCommandPacket_t;		//XBee Remote AT command packet

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
			union {
				psMessage_t message;
				uint8_t 	byteData[100];
			};
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
	struct {
		uint8_t apiIdentifier;
		uint8_t frameId;
		uint8_t ATcommand[2];
		uint8_t status;
	} packetHeader;
	union {
		struct {
			uint8_t byteData;
			uint8_t fill[3];
		};
		struct {
			uint16_t uint16Data;
			uint16_t fill16;
		};
		int 	intData;
	};
} ATResponse_t;					//XBee AT Command Response

typedef struct {
	struct {
		uint8_t 	apiIdentifier;	//0x97
		uint8_t 	frameId;
		uint8_t 	address64bit[8];	//responder
		uint8_t		address16bit[2];	//responder
		uint8_t 	ATcommand[2];
		uint8_t		status;
	} packetHeader;
	union {
		struct {
			uint8_t byteData;
			uint8_t fill[3];
		};
		struct {
			uint16_t uint16Data;
			uint16_t fill16;
		};
		int 	intData;
	};
} RemoteResponsePacket_t;				//XBee Remote AT command response

struct {
	XBeeTxStatus_enum 	status;
	uint8_t 			frameId;
} latestTxStatus;
pthread_cond_t txStatusCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t txStatusMutex = PTHREAD_MUTEX_INITIALIZER;

XBeeModemStatus_enum latestModemStatus;

TxPacket_t 				txPacket;		//regular Fido data packet (Tx)
RxPacket_t 				rxPacket;		//regular Fido data packet (Rx) + status packets

ATPacket_t 				atPacket;		//XBee AT command packet
ATResponse_t 			atResponse;	//XBee AT Command Response
int 					atResponseLength;

RemoteCommandPacket_t 	remoteCommand;	//XBee Remote AT command packet
RemoteResponsePacket_t 	remoteResponse;	//XBee Remote AT command response
int 					remoteResponseLength;

#define COND_TIMEOUT 5		//seconds
pthread_cond_t atResponseCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t atResponseMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t remoteResponseCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t remoteResponseMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t txMutex = PTHREAD_MUTEX_INITIALIZER;

int xbeeUartFD;
int txChecksum;
int rxChecksum;
uint8_t txSequenceNumber = 1;
uint8_t rxSequenceNumber;

int currentMonitorBattery 	= 1;
int currentMonitorLED 		= 1;
int currentCyclicSleep 		= 1;

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
char *modemStatusNames[] = MODEM_STATUS_NAMES;
char *commandStatusNames[] = COMMAND_STATUS_NAMES;

int EnterCommandMode();
bool inCommandMode = false;
int EnterAPIMode();
bool inApiMode = false;
int RemoteReset();
int SetRegister8(const char *atCommand, uint8_t value);
int SetRegister16(const char *atCommand, uint16_t value);
int GetRegister8(const char *atCommand);
int RemoteSetRegister8(const char *atCommand, uint8_t value);
int RemoteSetRegister16(const char *atCommand, uint16_t value);
int RemoteGetRegister8(const char *atCommand);

//API commands
int LocalCommand(const char *atCommand, uint8_t *param, int len, uint8_t *buf, int buflen );
int RemoteCommand(const char *atCommand, uint8_t *param, int len, uint8_t *buf, int buflen );

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
	gpio_set_value(XBEE_RESET_OUT_GPIO, 0);

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

//	bool xbeeReady = false;
//	int i;
//
//	while (1)
//	{
//		gpio_set_value(XBEE_RESET_OUT_GPIO, 1);
//		usleep(500000);
//		gpio_set_value(XBEE_RESET_OUT_GPIO, 0);		//reset XBee
//		usleep(500000);
//
//		if (EnterCommandMode() != 0)
//		{
//			SetCondition(XBEE_MCP_COMMS_ERRORS);
//			usleep(500000);
//			continue;
//		}
//
//		if (EnterAPIMode() != 0)
//		{
//			SetCondition(XBEE_MCP_COMMS_ERRORS);
//			usleep(500000);
//			continue;
//		}
//
//		xbeeReady = true;
//		break;
//	}
//
//	if (xbeeReady)
//	{
		CancelCondition(XBEE_MCP_COMMS_ERRORS);

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
//	}
//	else {
//		SetCondition(XBEE_MCP_COMMS_ERRORS);
//		return -1;
//	}
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

	if (SetRegister8(POWER_LEVEL, (int) powerLevel) < 0)
	{
		ERRORPRINT("Set Power Level fail\n");
	}

	int power = GetRegister8(POWER_LEVEL);
	if (power < 0)
	{
		ERRORPRINT("Read PowerLevel fail\n");
	} else {
		DEBUGPRINT("xbee power level = %i\n", power);
	}

	RemoteReset();
	sleep(1);

	DEBUGPRINT("XBee TX ready\n");

//	while(1)
//	{
//		if (RemoteSetRegister16(SAMPLE_RATE, 10000) < 0)
//		{
//			SetCondition(XBEE_MCP_COMMS_ERRORS);
//			ERRORPRINT("Failed to set Poll Interval\n");
//			usleep(500000);
//		}
//		else
//		{
//			break;
//		}
//	}

	for (;;) {
		struct timespec wait_time;

		//wait for a message
		psMessage_t *msg = GetNextMessage(&xbeeTxQueue);

		//Check for changes in XBee related options
		if (cyclicSleep != currentCyclicSleep)
		{
			if (cyclicSleep)
			{
//				if (SetRegister8(COORDINATOR_ENABLE, 1) < 0)
//				{
//					ERRORPRINT("Coordinator Enable fail\n");
//					SetCondition(XBEE_MCP_COMMS_ERRORS);
//				}
//
//				else if ((RemoteSetRegister16(CYCLIC_SLEEP_PERIOD, (int)(cyclicSleepPeriodS * 100)) < 0) ||
//						(SetRegister16(CYCLIC_SLEEP_PERIOD, (int)(cyclicSleepPeriodS * 100)) < 0))
//				{
//					ERRORPRINT("Failed to set Cyclic Sleep Period\n");
//					SetCondition(XBEE_MCP_COMMS_ERRORS);
//				}
//
//				else if ((RemoteSetRegister16(TIME_BEFORE_SLEEP, (int)(timeToSleepS * 1000)) < 0) ||
//						(SetRegister16(TIME_BEFORE_SLEEP, (int)(timeToSleepS * 1000)) < 0))
//				{
//					ERRORPRINT("Failed to set Time Before Sleep\n");
//					SetCondition(XBEE_MCP_COMMS_ERRORS);
//				}
//				else
				if (RemoteSetRegister8(SLEEP_MODE, 4) < 0)
				{
					ERRORPRINT("Failed to set Sleep Mode 4\n");
					SetCondition(XBEE_MCP_COMMS_ERRORS);
				}
//				else if (SetRegister8(SLEEP_OPTIONS, 0) < 0)
//				{
//					ERRORPRINT("Failed to set Sleep Options\n");
//					SetCondition(XBEE_MCP_COMMS_ERRORS);
//				}
				else
				{
					LogInfo("Cyclic Sleep enabled");
					currentCyclicSleep = cyclicSleep;
				}
			}
			else
			{
				if (RemoteSetRegister8(SLEEP_MODE, 0) < 0)
				{
					ERRORPRINT("Failed to set Sleep Mode 0\n");
					SetCondition(XBEE_MCP_COMMS_ERRORS);
				}
				else
				{
					LogInfo("Cyclic Sleep disabled");
					currentCyclicSleep = cyclicSleep;
				}
			}
		}

		if (monitorBattery != currentMonitorBattery)
		{
			if (monitorBattery)
			{
//				if (RemoteSetRegister16(SAMPLE_RATE, 10000) < 0)
//				{
//					SetCondition(XBEE_MCP_COMMS_ERRORS);
//					ERRORPRINT("Failed to set Poll Interval\n");
//				}
//				else
				if (RemoteSetRegister8(FIDO_XBEE_BATTERY, PIN_ADC) < 0)
				{
					SetCondition(XBEE_MCP_COMMS_ERRORS);
					ERRORPRINT("Failed to set Battery pin to ADC\n");
				}

				else
				{
					LogInfo("Battery Monitor enabled\n");
					currentMonitorBattery = monitorBattery;
				}
			}
			else
			{
				if (RemoteSetRegister8(FIDO_XBEE_BATTERY, PIN_DISABLED) < 0)
				{
					ERRORPRINT("Failed to set Battery pin to Disabled\n");
					SetCondition(XBEE_MCP_COMMS_ERRORS);
				}
				else
				{
					LogInfo("Battery Monitor disabled");
					currentMonitorBattery = monitorBattery;
				}
			}
		}

		if (monitorLED != currentMonitorLED)
		{
			if (monitorLED)
			{
//				if (RemoteSetRegister16(SAMPLE_RATE, (uint16_t) 10000) < 0)
//				{
//					ERRORPRINT("Failed to set Sample Rate\n");
//				}
//				else
				if (RemoteSetRegister8(FIDO_XBEE_LED, PIN_INPUT) < 0)
				{
					ERRORPRINT("Failed to set LED pin to Input\n");
				}
				else
				{
					LogInfo("LED Monitor enabled");
					currentMonitorLED = monitorLED;
				}
			}
			else
			{
				if (RemoteSetRegister8(FIDO_XBEE_LED, PIN_DISABLED) < 0)
				{
					ERRORPRINT("Failed to set LED pin to Disabled\n");
				}
				else
				{
					LogInfo("LED Monitor disabled");
					currentMonitorLED = monitorLED;
				}

			}
		}

		if (resetXBee)
		{
			gpio_set_direction(XBEE_RESET_OUT_GPIO, 0);	//output
			usleep(250000);
			gpio_set_value(XBEE_RESET_OUT_GPIO, 1);
			usleep(500000);
			gpio_set_value(XBEE_RESET_OUT_GPIO, 0);		//reset XBee
			sendOptionConfig("Reset XBee", 0, 0, 1, 0);
			LogInfo("Reset XBee");
		}

		if (shortPress || longPress)
		{
			if (RemoteSetRegister8(FIDO_XBEE_PWR_SW, PIN_OUTPUT_HIGH) < 0)
			{
				ERRORPRINT("Failed to set LED pin High\n");
			}
			else
			{
				if (longPress) {
					usleep((int)longPressS * 1000);
				}
				else
				{
					usleep((int)shortPressS * 1000);
				}

				//set pin low
				if (RemoteSetRegister8(FIDO_XBEE_PWR_SW, PIN_OUTPUT_LOW) < 0)
				{
					ERRORPRINT("Failed to set LED pin Low\n");
				}
				else
				{
					LogInfo("Pressed Power Button");

					if (shortPress) {
						shortPress = 0;
						sendOptionConfig("Short Press", 0, 0, 1, 0);
					}
					if (longPress) {
						longPress = 0;
						sendOptionConfig("Long Press", 0, 0, 1, 0);
					}
				}
			}
			sleep(1);
			//set disabled
			RemoteSetRegister8(FIDO_XBEE_PWR_SW, PIN_DISABLED);
		}

		DEBUGPRINT("XBee Tx: %s\n", psLongMsgNames[msg->header.messageType]);

		pthread_mutex_lock(&txMutex);

		pthread_mutex_lock(&txStatusMutex);

		if (SendFidoMessage(msg) == 0)
		{

			latestTxStatus.frameId = txSequenceNumber;
			latestTxStatus.status = XBEE_TX_TIMEOUT;

			clock_gettime(CLOCK_REALTIME, &wait_time);
			wait_time.tv_sec += XBEE_CYCLIC_SLEEP * 3;

			int reply = pthread_cond_timedwait(&txStatusCond, &txStatusMutex, &wait_time);

			if (reply == ETIMEDOUT)
			{
				ERRORPRINT("Send Fido Message timed out\n");
				SetCondition(XBEE_MCP_COMMS_ERRORS);
				sendErrors++;
			}
			else if (latestTxStatus.frameId != txSequenceNumber || latestTxStatus.status != XBEE_TX_SUCCESS)
			{
				ERRORPRINT("XBee TX %i Status: %s\n", txSequenceNumber, txStatusNames[latestTxStatus.status]);
				SetCondition(XBEE_MCP_COMMS_ERRORS);
				CancelCondition(XBEE_MCP_ONLINE);
				sendErrors++;
			}
			else
			{
				messagesSent++;
				CancelCondition(XBEE_MCP_COMMS_ERRORS);
			}
		} else {
			ERRORPRINT("XBee SendFidoMessage() error\n ");
			SetCondition(XBEE_MCP_COMMS_ERRORS);
			sendErrors++;
		}

		pthread_mutex_unlock(&txStatusMutex);

		pthread_mutex_unlock(&txMutex);

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
		return length;
}

//receives messages and passes to broker
void *RxThread(void *a) {
	psMessage_t *msg;

	DEBUGPRINT("XBee RX ready\n");

	for (;;) {
		int pktLength = ReadApiPacket();
		if (pktLength > 0)
		{

			switch(rxPacket.apiIdentifier)
			{
			case MODEM_STATUS:
				latestModemStatus = rxPacket.modemStatus.status;
				LogInfo("XBee Rx XBee status : %s", modemStatusNames[rxPacket.modemStatus.status]);
				break;
			case TRANSMIT_STATUS:
				DEBUGPRINT("XBee Rx Tx_Status: %s\n", txStatusNames[rxPacket.txStatus.status]);
				pthread_mutex_lock(&txStatusMutex);

				latestTxStatus.status = rxPacket.txStatus.status;
				latestTxStatus.frameId = rxPacket.txStatus.frameId;

				pthread_mutex_unlock(&txStatusMutex);
				pthread_cond_signal(&txStatusCond);
				break;
			case AT_RESPONSE:
				DEBUGPRINT("XBee Rx AT Response packet\n");
				pthread_mutex_lock(&atResponseMutex);

				memcpy(&atResponse, &rxPacket, sizeof(atResponse));
				atResponseLength = pktLength;

				pthread_mutex_unlock(&atResponseMutex);
				pthread_cond_signal(&atResponseCond);
				break;
			case REMOTE_CMD_RESPONSE:
				DEBUGPRINT("XBee Rx Remote Response packet\n");
				pthread_mutex_lock(&remoteResponseMutex);

				memcpy(&remoteResponse, &rxPacket, sizeof(remoteResponse));
				remoteResponseLength = pktLength;

				pthread_mutex_unlock(&remoteResponseMutex);
				pthread_cond_signal(&remoteResponseCond);
				break;
			case RECEIVE_16:
				SetCondition(XBEE_MCP_ONLINE);
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
						DEBUGPRINT("XBee Rx: %s\n", psLongMsgNames[msg->header.messageType]);
						//route the message
						NewBrokerMessage(msg);
						messagesReceived++;
					}
					else
					{
						DEBUGPRINT("XBee Rx Ignored: %s\n", psLongMsgNames[msg->header.messageType]);
						addressIgnored++;
					}
				}
				break;
			case RECEIVE_IO_16:
			{
				DEBUGPRINT("XBee Rx: IO_16\n");
				//data from remote XBee
				int length = pktLength - sizeof(rxPacket.fidoMessage.packetHeader);
				uint8_t *next = rxPacket.fidoMessage.byteData;
				int count = *next++;
				uint16_t channels = (*next++ << 8) + *next++;

				if (channels & IO_MSG_DIO_MASK)
				{
					//digital present
					uint16_t digitalIO = (*next++ << 8) + *next++;
					Condition(POWER_LED_ON , digitalIO & FIDO_XBEE_LED_MASK);

					DEBUGPRINT("XBee Rx: LED = %s\n", (digitalIO & FIDO_XBEE_LED_MASK ? "ON" : "OFF"));
				}
				int i;
				uint16_t mask = 0x200;
				for (i=0; i<6; i++)
				{
					if (channels & mask)
					{
						//channel present
						uint16_t value = (*next++ << 8) + *next++;

						if (i == FIDO_XBEE_BATTERY_CHAN)
						{
							psMessage_t msg;
							psInitPublish(msg, FLOAT_DATA);
							strcpy(msg.nameFloatPayload.name, "Battery");
							msg.nameFloatPayload.value = (float) (value * 3.2 / 1024.0) * 25.0 / 2.6125;
							RouteMessage(&msg);

							DEBUGPRINT("XBee Rx: Battery = %0.2f\n", msg.nameFloatPayload.value);
						}
					}
					mask = mask << 1;
				}

			}
			break;
			default:
				//ignore
				break;
			}
		}
		else
		{
			ERRORPRINT("XBee Rx: ReadApiPacket() Error");
			SetCondition(XBEE_MCP_COMMS_ERRORS);
		}
	}
	return 0;
}

//Used prior to changing to API mode
#define MAX_REPLY 10
int GetATReply(char *replyString)
{
	char *next = replyString;
	ssize_t len = MAX_REPLY-1;
	ssize_t reply;

	do {
		do {
			reply = read(xbeeUartFD, next, 1);
		} while (reply == 0 || reply == EAGAIN);
	} while (len-- > 0 && *next++ != '\r');

	*next = '\0';

	DEBUGPRINT("XBee AT Reply = %s\n", replyString);

	if (reply < 0) return -1;
	else return 0;
}

int SendATCommand(char *cmdString, char *replyString)
{
	ssize_t reply;

	if (!inCommandMode)
	{
		EnterCommandMode();
	}
	if (!inCommandMode) return -1;

	size_t len = strlen(cmdString);
	char *next = cmdString;

	while (len-- > 0)
	{
		WriteByte(*next++);
	}

	DEBUGPRINT("XBee Sent %s\n", cmdString);

	return GetATReply(replyString);
}

int RemoteReset()
{
	if (RemoteCommand(SOFTWARE_RESET, NULL, 0, NULL, 0) >= 0)
	{
		DEBUGPRINT("XBee Remote Reset\n");
		return 0;
	}
	else
	{
		ERRORPRINT("XBee Remote Reset fail\n");
		return -1;
	}
}

int SetRegister8(const char *atCommand, uint8_t value)
{

	uint8_t param = value;
	if (LocalCommand(atCommand, &param, 1, NULL, 0) >= 0)
	{
		DEBUGPRINT("XBee Set %s = %i\n", atCommand, value);
		return 0;
	}
	else
	{
		ERRORPRINT("XBee Set %s = %i fail\n", atCommand, value);
		return -1;
	}

}

int SetRegister16(const char *atCommand, uint16_t value)
{

	uint16_t param = value;
	if (LocalCommand(atCommand, (uint8_t*) &param, 2, NULL, 0) >= 0)
	{
		DEBUGPRINT("XBee Set %s = %i\n", atCommand, value);
		return 0;
	}
	else
	{
		ERRORPRINT("XBee Set %s = %i fail\n", atCommand, value);
		return -1;
	}

}

int GetRegister8(const char *atCommand)
{
	//	DEBUGPRINT("XBee Get %s\n", atCommand);

	uint8_t reply;
	if (LocalCommand(atCommand, NULL, 0, &reply, 1) == 1)
	{
		DEBUGPRINT("XBee Get %s = %i\n", atCommand, reply);
		return (int)reply;
	}
	else
	{
		ERRORPRINT("XBee Get %s fail\n", atCommand);
		return -1;
	}
}

int RemoteSetRegister8(const char *atCommand, uint8_t value)
{

	uint8_t param = value;
	if (RemoteCommand(atCommand, &param, 1, NULL, 0) >= 0)
	{
		DEBUGPRINT("XBee Remote Set %s = %i\n", atCommand, value);
		return 0;
	}
	else
	{
		ERRORPRINT("XBee Remote Set %s = %i fail\n", atCommand, value);
		return -1;
	}

}

int RemoteSetRegister16(const char *atCommand, uint16_t value)
{

	uint16_t param = value;
	if (RemoteCommand(atCommand, (uint8_t*) &param, 2, NULL, 0) >= 0)
	{
		DEBUGPRINT("XBee Remote Set %s = %i\n", atCommand, value);
		return 0;
	}
	else
	{
		ERRORPRINT("XBee Remote Set %s = %i fail\n", atCommand, value);
		return -1;
	}

}

int RemoteGetRegister8(const char *atCommand)
{
	//	DEBUGPRINT("XBee Remote Get %s\n", atCommand);

	uint8_t reply;
	if (RemoteCommand(atCommand, NULL, 0, &reply, 1) == 1)
	{
		DEBUGPRINT("XBee Remote Get %s = %i\n", atCommand, reply);
		return (int)reply;
	}
	else
	{
		ERRORPRINT("XBee Remote Get %s fail\n", atCommand);
		return -1;
	}
}


//sends atCommand with param bytes, if any. Copies response data, if any, into buf.
//reply: response length or -1
int LocalCommand(const char *atCommand, uint8_t *param, int len, uint8_t *buf, int buflen )
{
	int response, length;
	struct timespec absTime;

	length = len;
	if (length > 4) length = 4;
	if (++txSequenceNumber > 0xf0) txSequenceNumber = 1;

	pthread_mutex_lock(&txMutex);

	atPacket.packetHeader.apiIdentifier 	= AT_COMMAND;
	atPacket.packetHeader.frameId			= txSequenceNumber;
	atPacket.packetHeader.ATcommand[0] 		= atCommand[0];
	atPacket.packetHeader.ATcommand[1] 		= atCommand[1];

	memcpy(&atPacket.byteData, param, length);

	SendApiPacket((uint8_t*)&atPacket, sizeof(atPacket.packetHeader) + length);

	clock_gettime(CLOCK_REALTIME, &absTime);
	absTime.tv_sec += 5;

	int reply = pthread_cond_timedwait(&atResponseCond, &atResponseMutex, &absTime);

	if (reply == ETIMEDOUT)
	{
		ERRORPRINT("API Call timed out\n");
		response = -1;
	}
	else if (atResponse.packetHeader.status == COMMAND_OK) {
		DEBUGPRINT("Sent API Pkt %s OK\n", atCommand);
		response = atResponseLength - sizeof(atResponse.packetHeader);
		if (response > buflen) response = buflen;
		memcpy(buf, &atResponse.byteData, response);
	}
	else {
		ERRORPRINT("API Call failed: %s\n", commandStatusNames[atResponse.packetHeader.status]);
		response = -1;
	}

	pthread_mutex_unlock(&atResponseMutex);

	pthread_mutex_unlock(&txMutex);

	return response;
}

//remote version
//sends atCommand with param bytes, if any. Copies response data, if any, into buf.
int RemoteCommand(const char *atCommand, uint8_t *param, int len, uint8_t *buf, int buflen )
{
	int response, length;
	struct timespec absTime;

	length = len;
	if (length > 4) length = 4;
	if (++txSequenceNumber > 0xf0) txSequenceNumber = 1;

	pthread_mutex_lock(&txMutex);

	remoteCommand.packetHeader.apiIdentifier 	= REMOTE_AT_COMMAND;
	remoteCommand.packetHeader.frameId			= txSequenceNumber;
	remoteCommand.packetHeader.commandOptions	= 0x02;
	remoteCommand.packetHeader.destinationMSB  	= 0;
	remoteCommand.packetHeader.destinationLSB  	= MCP_ADDRESS;
	remoteCommand.packetHeader.ATcommand[0] 	= atCommand[0];
	remoteCommand.packetHeader.ATcommand[1] 	= atCommand[1];

	memcpy(&remoteCommand.byteData, param, length);

	SendApiPacket((uint8_t*)&remoteCommand, sizeof(remoteCommand.packetHeader) + length);

	clock_gettime(CLOCK_REALTIME, &absTime);
	absTime.tv_sec += XBEE_CYCLIC_SLEEP * 3;

	int reply = pthread_cond_timedwait(&remoteResponseCond, &remoteResponseMutex, &absTime);

	if (reply == ETIMEDOUT)
	{
		ERRORPRINT("Remote API Call timed out\n");
		response = -1;
	}
	else if (remoteResponse.packetHeader.status == COMMAND_OK) {
		DEBUGPRINT("Sent Remote API Pkt %s OK\n", atCommand);
		response = remoteResponseLength - sizeof(remoteResponse.packetHeader);
		if (response > buflen) response = buflen;
		memcpy(buf, &remoteResponse.byteData, response);
	}
	else {
		ERRORPRINT("Remote API Call failed: %s\n", commandStatusNames[remoteResponse.packetHeader.status]);
		response = -1;
	}

	pthread_mutex_unlock(&remoteResponseMutex);

	pthread_mutex_unlock(&txMutex);

	return response;
}

//tx power level
int SetPowerLevel(int pl)
{
	return SetRegister8(POWER_LEVEL, pl);
}
int GetPowerLevel()
{
	return GetRegister8(POWER_LEVEL);
}

int EnterCommandMode()
{
	int reply;
	char replyString[MAX_REPLY];

	tcflush(xbeeUartFD, TCIOFLUSH);

	usleep(1500000);

	WriteByte('+');
	WriteByte('+');
	WriteByte('+');

	reply = GetATReply(replyString);

	DEBUGPRINT("Command Mode: %s\n", replyString);

	usleep(500000);

	if (reply < 0 || strncmp(replyString, "OK", 2) != 0)
	{
		inCommandMode = false;
		return -1;
	}
	else {
		inCommandMode = true;
		return 0;
	}
}
int EnterAPIMode()
{
	int reply;

	char replyString[MAX_REPLY];

	reply = SendATCommand("ATAP2\r", replyString);

	if (reply < 0 || strncmp(replyString, "OK", 2) != 0)
	{
		ERRORPRINT("API Mode set fail\n");
		return -1;
	}

	DEBUGPRINT("API Mode Set\n");

	inApiMode = true;

	return reply;
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

