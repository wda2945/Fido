//
//  MessageLengths.c
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 4/21/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//
#include "string.h"
#include "PubSubData.h"

void AdjustMessageLength (psMessage_t *_msg) {
    
    if (_msg->header.messageType >= PS_MSG_COUNT) return;
    
    int _msgLen = psMessageFormatLengths[psMsgFormats[_msg->header.messageType]];
    
    //fix variable length payloads
    if (_msgLen < 0) {
        size_t _textLen;
        switch (psMsgFormats[_msg->header.messageType]) {
            case PS_SYSLOG_PAYLOAD:
                _msg->logPayload.text[PS_MAX_LOG_TEXT] = 0;
                _textLen = strlen(_msg->logPayload.text);
                if (_textLen > (PS_MAX_LOG_TEXT)) _textLen = PS_MAX_LOG_TEXT;
                break;
            case PS_NAME_PAYLOAD:
                _msg->namePayload.name[PS_NAME_LENGTH] = 0;
                _textLen = strlen(_msg->namePayload.name);
                if (_textLen > (PS_NAME_LENGTH)) _textLen = PS_NAME_LENGTH;
                break;
            case PS_NAME_BYTE_PAYLOAD:
                _msg->nameBytePayload.name[PS_NAME_LENGTH] = 0;
                _textLen = strlen(_msg->nameBytePayload.name);
                if (_textLen > (PS_NAME_LENGTH)) _textLen = PS_NAME_LENGTH;
                break;
            case PS_NAME_FLOAT_PAYLOAD:
                _msg->nameFloatPayload.name[PS_NAME_LENGTH] = 0;
                _textLen = strlen(_msg->nameFloatPayload.name);
                if (_textLen > (PS_NAME_LENGTH)) _textLen = PS_NAME_LENGTH;
                break;
            case PS_NAME_INT_PAYLOAD:
                _msg->nameIntPayload.name[PS_NAME_LENGTH] = 0;
                _textLen = strlen(_msg->nameIntPayload.name);
                if (_textLen > (PS_NAME_LENGTH)) _textLen = PS_NAME_LENGTH;
                break;
            case PS_NAME_4BYTE_PAYLOAD:
                _msg->name4BytePayload.name[PS_NAME_LENGTH] = 0;
                _textLen = strlen(_msg->name4BytePayload.name);
                if (_textLen > (PS_NAME_LENGTH)) _textLen = PS_NAME_LENGTH;
                break;
            case PS_NAME_3INT_PAYLOAD:
                _msg->name3IntPayload.name[PS_NAME_LENGTH] = 0;
                _textLen = strlen(_msg->name3IntPayload.name);
                if (_textLen > (PS_NAME_LENGTH)) _textLen = PS_NAME_LENGTH;
                break;
            case PS_NAME_3FLOAT_PAYLOAD:
                _msg->name3FloatPayload.name[PS_NAME_LENGTH] = 0;
                _textLen = strlen(_msg->name3FloatPayload.name);
                if (_textLen > (PS_NAME_LENGTH)) _textLen = PS_NAME_LENGTH;
                break;
            case PS_TASK_STATS_PAYLOAD:
                _msg->taskStatsPayload.taskName[PS_NAME_LENGTH] = 0;
                _textLen = strlen(_msg->taskStatsPayload.taskName);
                if (_textLen > (PS_NAME_LENGTH)) _textLen = PS_NAME_LENGTH;
                break;
            case PS_SETTING_PAYLOAD:
                _msg->settingPayload.name[PS_NAME_LENGTH] = 0;
                _textLen = strlen(_msg->settingPayload.name);
                if (_textLen > (PS_NAME_LENGTH)) _textLen = PS_NAME_LENGTH;
            	break;
            case PS_OPTION_PAYLOAD:
                _msg->optionPayload.name[PS_NAME_LENGTH] = 0;
                _textLen = strlen(_msg->optionPayload.name);
                if (_textLen > (PS_NAME_LENGTH)) _textLen = PS_NAME_LENGTH;
                break;
            default:
            	//must be taken care of
                _textLen = _msg->header.length + _msgLen;
                break;
        }
        _msg->header.length = _textLen - _msgLen;
    }
    else{
        _msg->header.length = _msgLen;
    }
}
