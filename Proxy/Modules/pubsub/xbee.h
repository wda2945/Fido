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
#define REMOTE_AT_COMMAND	0x17
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
#define TIME_BEFORE_SLEEP	"ST"
#define CYCLIC_SLEEP_PERIOD	"SP"
#define API_ENABLE			"AP"
#define FIRMWARE_VERSION	"VR"
#define HARDWARE_VERSION	"HV"
#define SIGNAL_STRENGTH		"DB"
#define CCA_FAILURES		"EC"
#define ACK_FAILURES		"EA"
#define COORDINATOR_ENABLE	"CE"
#define SAMPLE_RATE			"IR"
#define SOFTWARE_RESET		"FR"

#define PIN_DISABLED		0
#define PIN_ADC				2
#define PIN_INPUT			3
#define PIN_OUTPUT_LOW		4
#define PIN_OUTPUT_HIGH		5

#define IO_MSG_DIO_MASK			0x1ff	//mask for digital pins
#define IO_MSG_ADC_MASK			0x7e00	//mask for analog pins

//TX Status
typedef enum { XBEE_TX_SUCCESS, XBEE_TX_NO_ACK, XBEE_TX_CCA_FAIL, XBEE_TX_PURGED, XBEE_TX_TIMEOUT} XBeeTxStatus_enum;
#define TX_STATUS_NAMES {"OK", "No Ack", "CCA Fail", "Tx Purged", "Tx Timeout"}

//Modem Status
typedef enum {XBEE_HW_RESET, XBEE_WATCHDOG_RESET, XBEE_ASSOCIATED, XBEE_DISASSOCIATED, XBEE_SYNC_LOST, XBEE_COORDINATOR_REALIGNMENT, XBEE_COORDINATOR_STATRTED} XBeeModemStatus_enum;
#define MODEM_STATUS_NAMES {"HW Reset", "WD_Reset", "Associated", "Disassociated", "Sync Lost", "Coord Realignment", "Coord Started"}

//Remote Command Response Status
typedef enum {COMMAND_OK, COMMAND_ERROR, COMMAND_INVALID, COMMAND_PARAMETER_INVALID, COMMAND_NO_RESPONSE } XBEERemoteResponseStatus;
#define COMMAND_STATUS_NAMES {"OK", "Error", "Command Invalid", "Parameter Invalid", "No Response"}

int SetPowerLevel(int pl);
int GetPowerLevel();

#endif
