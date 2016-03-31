//
//  SoftwareProfile.h
//  Fido
//
//  Created by Martin Lane-Smith on 6/17/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#ifndef SoftwareProfile_h
#define SoftwareProfile_h

//FIDO version
#define FIDO
#define BBB

#define MAIN_DEBUG

//enabled subsystem debug
//#define BROKER_DEBUG
#define NOTIFICATIONS_DEBUG
//#define AGENT_DEBUG

#define BEHAVIOR_DEBUG
#define BEHAVIOR_TREE_DEBUG
#define AUTOPILOT_DEBUG

//#define NAVIGATOR_DEBUG
//#define GPS_DEBUG
//#define IMU_DEBUG

//#define SCANNER
//#define SCANNER_DEBUG

//#define CAMERA
//#define CAMERA_DEBUG

//#define TTS
//#define TTS_DEBUG

//#define IR_PINGER
//#define IR_PINGER_DEBUG

#define BEHAVIOR_TREE_CLASS	"/root/lua/behaviortree/behaviour_tree.lua"
#define INIT_SCRIPT_PATH 	"/root/lua/init"				//initialization
#define BT_LEAF_PATH		"/root/lua/bt_leafs"
#define BT_ACTIVITY_PATH	"/root/lua/bt_activities"
#define HOOK_SCRIPT_PATH 	"/root/lua/hooks"				//Message hooks
#define GENERAL_SCRIPT_PATH "/root/lua/scripts"				//General scripts

#define WAYPOINT_FILE_PATH	"/root/data/waypoints.xml"
#define SAVED_SETTINGS_FILE "/root/data/settings.txt"
#define SAVED_OPTIONS_FILE "/root/data/options.txt"

#define SPARE_ANALOG0		AN0
#define SPARE_ANALOG1		AN1

//UART broker
#define PS_UART_DEVICE 		"/dev/ttyO5"
#define PS_TX_PIN				"P8_37"
#define PS_RX_PIN				"P8_38"
#define PS_UART_BAUDRATE 	B115200

//GPS
#define GPS_UART_DEVICE 	"/dev/ttyO2"
#define GPS_TX_PIN				"P9_21"
#define GPS_RX_PIN				"P9_22"
#define GPS_UART_BAUDRATE 	B9600

//CAMERA
#define CAMERA_UART_DEVICE 	"/dev/ttyO1"
#define CAM_TX_PIN				"P9_24"
#define CAM_RX_PIN				"P9_26"
#define CAMERA_UART_BAUDRATE 	B38400

//TTS
#define TTS_UART_DEVICE 	"/dev/ttyO3"
#define TTS_TX_PIN				"P9_42"
#define TTS_RX_PIN				""
#define TTS_UART_BAUDRATE 	B9600

//IMU
#define IMU_I2C           		1
#define IMU_SCL_PIN				""
#define IMU_SDA_PIN				""
#define IMU_XM_I2C_ADDRESS   	0x1D
#define IMU_G_I2C_ADDRESS   	0x6B

//IR PINGER
#define IR_UART_DEVICE 		"/dev/ttyO4"
#define IR_TX_PIN				"P9_13"
#define IR_RX_PIN				"P9_11"
#define IR_UART_BAUDRATE 	B57600
#define IR_PWM_KEY   		"P9_14"
#define IR_PWM_EXPORT		1
#define IR_PWM_DIR			"pwm1"
#define IR_PWM_FREQUENCY	38000.0		//Hz
#define IR_PWM_PERIOD		(1000.0/IR_PWM_FREQUENCY)	//ms

//I2C Lidar
#define LIDAR_I2C           		2
#define LIDAR_SCL_PIN				"P9_17"
#define LIDAR_SDA_PIN				"P9_18"
#define LIDAR_I2C_ADDRESS   		0x62
#define RegisterMeasure    			0x00          // Register to write to initiate ranging.
#define MeasureValue        		0x04          // Value to initiate ranging.
#define RegisterHighLowB    		0x8f          // Register to get both High and Low bytes in 1 call.
#define LIDAR_RANGE_WAIT_NS 		1000000000	//1000mS
#define LIDAR_HALF_BEAM_WIDTH		5
#define LIDAR_FALSE_POSITIVE		0.05f
#define LIDAR_FALSE_NEGATIVE_20		0.33f
#define LIDAR_FALSE_NEGATIVE_200	0.5f

//Servo
#define SERVO_PWM_KEY   		"P9_29"
#define SERVO_PWM_EXPORT		1
#define SERVO_PWM_DIR			"pwm1"

//servo driving
#define SERVO_FREQUENCY			100.0		//Hz
#define MIN_PULSE				1.0			//ms
#define MAX_PULSE				2.0			//ms
#define MID_PULSE				1.5			//ms
#define MIN_MOVE_MS				50
#define MS_PER_STEP				10			//servo move time

//logfiles folder
#define LOGFILE_FOLDER "/root/logfiles"
//Logging level
#define SYSLOG_LEVEL                LOG_ALL  	//published log

//queueing limits for serial
#define MAX_ROUTINE 1
#define MAX_INFO 3
#define MAX_WARNING 5
#define MAX_ERROR 10

#endif
