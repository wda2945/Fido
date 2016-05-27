//
//  MsgMacros.h
//  Common
//
//  Created by Martin Lane-Smith on 1/25/14.
//  Copyright (c) 2013 Martin Lane-Smith. All rights reserved.
//

//#define messagemacro(enum, qos, topic, payload, long name)

messagemacro(PS_UNSPECIFIED,PS_QOS3,PS_UNDEFINED_TOPIC,PS_UNKNOWN_PAYLOAD,"Unknown")

//logging
messagemacro(SYSLOG_MSG,PS_QOS3,LOG_TOPIC,PS_SYSLOG_PAYLOAD,"Sys Log")
messagemacro(BBBLOG_MSG,PS_QOS3,OVM_LOG_TOPIC,BBB_SYSLOG_PAYLOAD,"BBB Log")

//system management (generally down the chain of command)
messagemacro(PING_MSG,PS_QOS2,ANNOUNCEMENTS_TOPIC,PS_REQUEST_PAYLOAD,"Ping")
messagemacro(PING_RESPONSE,PS_QOS2,RESPONSE_TOPIC,PS_RESPONSE_PAYLOAD,"Ping response")      	//I am alive

messagemacro(TICK_1S,PS_QOS2,TICK_1S_TOPIC,PS_TICK_PAYLOAD,"1S Tick")

//state management
messagemacro(SS_ONLINE,PS_QOS3,RESPONSE_TOPIC,PS_RESPONSE_PAYLOAD,"SS Online")        		//I am alive
messagemacro(POWER_STATE,PS_QOS3,RESPONSE_TOPIC,PS_BYTE_PAYLOAD,"Power State")
messagemacro(COMMAND,PS_QOS1,MCP_ACTION_TOPIC,PS_BYTE_PAYLOAD,"App Command")	 	  			//state command from App
messagemacro(OVM_COMMAND,PS_QOS1,MCP_ACTION_TOPIC,PS_BYTE_PAYLOAD,"OVM Command")				//request power state

//Notifications
messagemacro(WAKE_MASK,PS_QOS1,MCP_ACTION_TOPIC,PS_MASK_PAYLOAD,"Wake Mask")					//WAKE MASK
messagemacro(NOTIFY,PS_QOS1,EVENTS_TOPIC,PS_INT_PAYLOAD,"Notify")                               //Event
messagemacro(CONDITIONS,PS_QOS1,CONDITIONS_TOPIC,PS_MASK_PAYLOAD,"Conditions")                //Conditions

//config
messagemacro(CONFIG,PS_QOS2,ANNOUNCEMENTS_TOPIC,PS_CONFIG_PAYLOAD,"Send Config")            //send available settings and options
messagemacro(SETTING,PS_QOS2,RESPONSE_TOPIC,PS_SETTING_PAYLOAD,"Setting")
messagemacro(OPTION,PS_QOS2,RESPONSE_TOPIC,PS_OPTION_PAYLOAD,"Option")
messagemacro(CONFIG_DONE,PS_QOS2,RESPONSE_TOPIC,PS_CONFIG_PAYLOAD,"Config Sent")       		//sent available settings and options
messagemacro(NEW_SETTING,PS_QOS1,ANNOUNCEMENTS_TOPIC,PS_SETTING_PAYLOAD,"New Setting")      //change setting (float)
messagemacro(SET_OPTION,PS_QOS1,ANNOUNCEMENTS_TOPIC,PS_OPTION_PAYLOAD,"Set Option")         //change option  (int/bool)

//PIC stats
messagemacro(GEN_STATS,PS_QOS3,ANNOUNCEMENTS_TOPIC,PS_NO_PAYLOAD,"Gen Stats")
messagemacro(TASK_STATS,PS_QOS3,APP_REPORT_TOPIC,PS_TASK_STATS_PAYLOAD,"Task Stats")
messagemacro(SYS_STATS,PS_QOS3,APP_REPORT_TOPIC,PS_SYS_STATS_PAYLOAD,"Sys Stats")
messagemacro(COMMS_STATS,PS_QOS3,APP_REPORT_TOPIC,PS_COMMS_STATS_PAYLOAD,"Comms Stats")

//environment reports
messagemacro(BATTERY,PS_QOS2,SYS_REPORT_TOPIC,PS_BATTERY_PAYLOAD,"Battery")
messagemacro(ENVIRONMENT,PS_QOS3,SYS_REPORT_TOPIC,PS_ENVIRONMENT_PAYLOAD,"Environment")

//General data reports (-> App)
messagemacro(FLOAT_DATA,PS_QOS3,APP_REPORT_TOPIC,PS_NAME_FLOAT_PAYLOAD,"fData")
messagemacro(INT_DATA,PS_QOS3,APP_REPORT_TOPIC,PS_NAME_INT_PAYLOAD,"iData")

//raw sensors (sensor -> overmind)
messagemacro(GPS_REPORT,PS_QOS3,RAW_NAV_TOPIC,PS_POSITION_REPORT_PAYLOAD,"GPS")
messagemacro(IMU_REPORT,PS_QOS3,RAW_NAV_TOPIC,PS_3FLOAT_PAYLOAD,"IMU")
messagemacro(ODOMETRY,PS_QOS1,RAW_ODO_TOPIC,PS_ODOMETRY_PAYLOAD,"Odometry")

//Proximity
messagemacro(PROXREP,PS_QOS2,RAW_PROX_TOPIC,PS_PROX_SUMMARY_PAYLOAD,"Prox Report")

//cooked sensors
messagemacro(POSE,PS_QOS2,NAV_REPORT_TOPIC,PS_POSE_PAYLOAD,"Pose")
messagemacro(POSEREP,PS_QOS2,APP_REPORT_TOPIC,PS_POSE_PAYLOAD,"Poserep")						//version for APP

//raw movement -> motors
messagemacro(MOTORS,PS_QOS1,MOT_ACTION_TOPIC,PS_MOTOR_PAYLOAD,"Motors")

//OVM reports (-> App)
messagemacro(ACTIVITY,PS_QOS1,APP_REPORT_TOPIC,PS_BEHAVIOR_STATUS,"Active Behavior")
messagemacro(SCRIPT,PS_QOS1,APP_REPORT_TOPIC,PS_NAME_PAYLOAD,"Available Script")

//OVM script control
messagemacro(ACTIVATE,PS_QOS1,OVM_ACTION_TOPIC,PS_NAME_PAYLOAD,"Execute Script")
messagemacro(RELOAD,PS_QOS1,OVM_ACTION_TOPIC,PS_NO_PAYLOAD,"Reload Scripts")
messagemacro(HEEL_LOCATION,PS_QOS1,OVM_ACTION_TOPIC,PS_POSITION_REPORT_PAYLOAD,"Heel Position")

//Alert
messagemacro(ALERT,PS_QOS2,APP_REPORT_TOPIC,PS_NAME_PAYLOAD,"Alert")

messagemacro(KEEPALIVE,PS_QOS2,PS_UNDEFINED_TOPIC,PS_NO_PAYLOAD,"Keepalive")

//#undef messagemacro
