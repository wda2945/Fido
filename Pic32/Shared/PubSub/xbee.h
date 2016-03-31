//xbee.h

#ifndef XBEE_H
#define XBEE_H

//API Mode

//escaped characters
#define FRAME_DELIMITER 0x7e
#define ESCAPE			0x70
#define XON				0x11
#define XOFF			0x12
#define ESCAPEE			0x7d
#define CHAR_XOR		0x20

//API Identifiers
#define MODEM_STATUS		0x8A
#define AT_COMMAND			0x08
#define QUEUE_PARAM_VALUE	0x09
#define AT_RESPONSE			0x88
#define AT_COMMAND_REQUEST	0x17
#define REMOTE_CMD_RESPONSE	0x97
#define TRANSMIT_64			0x00
#define TRANSMIT_16			0x01
#define TRANSMIT_STATUS		0x89
#define RECEIVE_64			0x80
#define RECEIVE_16			0x81
#define RECEIVE_IO_64		0x82
#define RECEIVE_IO_16		0x83

//AT Commands
#define POWER_LEVEL			"PL"
#define SLEEP_MODE 			"SM"
#define SLEEP_OPTIONS		"SO"
#define TIME_TO_SLEEP		"ST"
#define CYCLIC_SLEEP_PERIOD	"SP"
#define API_ENABLE			"AP"
#define FIRMWARE_VERSION	"VR"
#define HARDWARE_VERSION	"HV"
#define SIGNAL_STRENGTH		"DB"
#define CCA_FAILURES		"EC"
#define ACK_FAILURES		"EA"

//TX Status
typedef enum { XBEE_TX_SUCCESS, XBEE_TX_NO_ACK, XBEE_TX_CCA_FAIL, XBEE_TX_PURGED} XBeeTxStatus_enum;

#define TX_STATUS_NAMES {"OK", "No Ack", "CCA Fail", "Tx Purged"}

//Modem Status
typedef enum {XBEE_HW_RESET, XBEE_WATCHDOG_RESET, XBEE_ASSOCIATED, XBEE_DISASSOCIATED, XBEE_SYNC_LOST, XBEE_COORDINATOR_REALIGNMENT, XBEE_COORDINATOR_STATRTED} XBeeModemStatus_enum;

int SetPowerLevel(int pl);
int GetPowerLevel();


#endif
