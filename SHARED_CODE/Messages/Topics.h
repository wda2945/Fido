/* 
 * File:   Topics.h
 * Author: martin
 *
 * Created on June 14, 2014, 9:19 PM
 */

#ifndef TOPICS_H
#define	TOPICS_H

//---------------------Topics

//Subscribable Topics

typedef enum {
    PS_UNDEFINED_TOPIC,
    
    LOG_TOPIC,              //SysLog
    ANNOUNCEMENTS_TOPIC,    //Broadcast channel	ALL
    RESPONSE_TOPIC,			//Ping/Config replies ALL->APP/OVM
       
    APP_REPORT_TOPIC,       //To send data to APP only
	SYS_REPORT_TOPIC,       //Reports to APP & Overmind
	
    RAW_ODO_TOPIC,          //Raw Odometry 		MOT->OWVM
    RAW_PROX_TOPIC,         //Raw Prox reports 	MCP->OVM
    RAW_NAV_TOPIC,          //Raw IMU			OVM->OVM
    
    TICK_1S_TOPIC,          //1 second ticks	MCP->MOT,OVM
    
    MCP_ACTION_TOPIC,       //commands to MCP	APP/OVM->MCP
    MOT_ACTION_TOPIC,       //Move commands		APP/OVM->MOT
    OVM_ACTION_TOPIC,       //OVM				APP->OVM
    
    NAV_REPORT_TOPIC,       //Fused Nav Reports OVM->OVM
    
    OVM_LOG_TOPIC,          //internal logs		OVM->OVM
            
    EVENTS_TOPIC,           //Notify			MOT/MCP/OVM
    CONDITIONS_TOPIC,		//Conditions		ALL
            
    PS_TOPIC_COUNT
} psTopic_enum;

#define PS_TOPIC_NAMES { \
    "UNDEFINED",\
    "SYSLOG",\
    "ANNOUNCE",\
    "RESPONSE",\
    "TO_APP",\
	"SYS_REPORT",\
    "RAW ODO",\
    "RAW PROX",\
    "RAW NAV",\
    "TICK_1S",\
    "MCP_ACTION",\
    "MOT ACTION",\
    "OVM_ACTION",\
    "NAV_REPORT",\
    "OVM LOG",\
    "EVENTS"\
    "CONDITIONS"\
    }

extern char *psTopicNames[PS_TOPIC_COUNT];

#endif	/* TOPICS_H */

