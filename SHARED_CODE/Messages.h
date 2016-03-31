/* 
 * File:   Messages.h
 * Author: martin
 *
 * Enums and structs of Messages
 *
 */

#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include <stdint.h>

//---------------------Message Enums

#include "Messages/MessageEnums.h"

//---------------------Message Formats

#include "Messages/MessageFormats.h"

//---------------------Message Topics

#include "Messages/Topics.h"

//---------------------Message codes enum

#define messagemacro(m,q,t,f,l) m,
typedef enum {
#include "Messages/MessageList.h"
    PS_MSG_COUNT
} psMessageType_enum;
#undef messagemacro

//---------------------Message Formats enum

#define formatmacro(e,t,p,s) e,

typedef enum {
#include "Messages/MsgFormatList.h"
    PS_FORMAT_COUNT
} psMsgFormat_enum;
#undef formatmacro

//----------------------Complete message struct

//Generic struct for all messages

#define formatmacro(e,t,p,s) t p;
#pragma pack(1)
typedef struct {
    psMessageHeader_t header;
    //Union option for each payload type
    union {
        uint8_t packet[1];
#include "Messages/MsgFormatList.h"
    };
} psMessage_t;
#pragma pack()
#undef formatmacro

#endif
