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

#include "messages/messageenums.h"

//---------------------Message Formats

#include "messages/messageformats.h"

//---------------------Message Topics

#include "messages/topics.h"

//---------------------Message codes enum

#define messagemacro(m,q,t,f,l) m,
typedef enum {
#include "messages/messagelist.h"
    PS_MSG_COUNT
} psMessageType_enum;
#undef messagemacro

//---------------------Message Formats enum

#define formatmacro(e,t,p,s) e,

typedef enum {
#include "messages/msgformatlist.h"
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
#include "messages/msgformatlist.h"
    };
} psMessage_t;
#pragma pack()
#undef formatmacro

#endif
