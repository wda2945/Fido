//
//  SoftwareProfile.h
//  Fido
//
//  Created by Martin Lane-Smith on 6/17/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#ifndef SoftwareProfile_h
#define SoftwareProfile_h

#define MAIN_DEBUG

//enabled subsystem debug
#define BROKER_DEBUG
#define NOTIFICATIONS_DEBUG
#define AGENT_DEBUG

//XBee broker
#define PAN_ID			0x3332
#define MCP_ADDRESS 	0x5A
#define PROXY_ADDRESS	0x1A

#define PS_UART_DEVICE 		"/dev/ttyO2"
#define PS_TX_PIN			"P9_21"
#define PS_RX_PIN			"P9_22"

#define PS_UART_BAUDRATE 	B115200

//local XBEE
#define XBEE_RESET			"P8-33"
#define XBEE_RESET_GPIO		9
#define XBEE_RESET_OUT		"P9-27"
#define XBEE_RESET_OUT_GPIO	115
#define XBEE_DTR			"P8-26"
#define XBEE_DTR_GPIO		86
#define XBEE_STATUS			"P9-28"
#define XBEE_STATUS_GPIO	113
#define XBEE_ASSOCIATE		"P9-30"
#define XBEE_ASSOCIATE_GPIO	112

//remote XBEE
#define FIDO_XBEE_BATTERY	"D0"
#define FIDO_XBEE_BATTERY_CHAN 0
#define FIDO_XBEE_PWR_SW	"D1"
#define FIDO_XBEE_LED		"D2"
#define FIDO_XBEE_LED_MASK	0x4

#define XBEE_SAMPLE_INTERVAL 10		//s
#define XBEE_CYCLIC_SLEEP	 15		//s
#define XBEE_TIME_TO_SLEEP	 15		//s

//logfiles folder
#define LOGFILE_FOLDER "/root/logfiles/"

//Logging level
#define SYSLOG_LEVEL                LOG_ALL  	//published log


#endif
