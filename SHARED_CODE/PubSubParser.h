/* 
 * File:   PubSubParser.h
 * Author: martin
 *
 * General parser state machine for pulling messages off serial links
 *
 * Created on December 28, 2013, 5:23 PM
 */

#ifndef PUBSUBPARSER_H
#define	PUBSUBPARSER_H

#include <stdint.h>
#include "Messages.h"

#define STX_CHAR 0x7f

typedef enum {
    PARSE_STATE_UNINIT=0,
    PARSE_STATE_IDLE,
    PARSE_STATE_GOT_STX,
    PARSE_STATE_GOT_LENGTH1,
    PARSE_STATE_GOT_LENGTH2,
    PARSE_STATE_GOT_SEQ,
    PARSE_STATE_GOT_SRCID,
    PARSE_STATE_GOT_MSGID,
    PARSE_STATE_GOT_TOPICID,
    PARSE_STATE_GOT_PAYLOAD,
    PARSE_STATE_GOT_CRC1
} ParseState_enum; ///< The state machine for the comm parser

typedef enum {
    PARSE_OK,
    PARSED_MESSAGE,
    PARSE_LENGTH_MISMATCH,
    PARSE_SEQUENCE_ERROR,
    PARSE_BAD_MESSAGE_TYPE,
    PARSE_BAD_MESSAGE_LENGTH,
    PARSE_CHECKSUM,
    PARSE_RESULT_COUNT
} ParseResult_enum;
#define PARSE_RESULTS {"Running", "Message", "Length Mis-match", "Sequence Error", "Bad Type", "Bad Message Length", "Checksum"}

typedef struct __status {
    ParseResult_enum parse_result;               
    ParseState_enum parse_state;          ///< Parsing state machine
    uint32_t packet_rx_success_count;   ///< Received packets
    uint32_t packet_rx_drop_count;      ///< Number of packet drops
    int16_t checksum;                   ///< Running checksum
    uint8_t parse_error;                ///< Number of parse errors
    uint8_t packet_idx;                 ///< Index in current packet
    uint8_t current_rx_seq;             ///< Sequence number of last packet received
    uint8_t	noCRC;						///< Do not expect a CRC, if > 0
    uint8_t noSeq;						///< Do not check seq #s, if > 0
    uint8_t noLength2;                  ///< Do not check for duplicate length, if > 0
    uint8_t noTopic;                    ///< Do not check for topic ID, if > 0
} status_t;


#ifdef	__cplusplus
extern "C" {
#endif

//init data struct
void ResetParseStatus(status_t *status);

//Serial input parser

ParseResult_enum ParseNextCharacter(uint8_t c, psMessage_t *msg, status_t *status);



#ifdef	__cplusplus
}
#endif

#endif	/* PUBSUBPARSER_H */

