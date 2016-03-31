/*
 * File:   PubSubParser.c
 * Author: martin
 *
 * Created on December 28, 2013
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "PortablePrint.h"

#include "PubSubData.h"
#include "PubSubParser.h"

//#define DEBUGPRINT(...) DebugPrint(__VA_ARGS__)
#define DEBUGPRINT(...)

//Parses messages from UART, etc.

//message checksum
#define start_checksum(s) s->checksum = 0
#define update_checksum(s,c) s->checksum += c
#define verify_checksum(s,c) ((s->checksum & 0xff) == c)

//adds character to the message
//returns TRUE if msg is a complete message

ParseResult_enum ParseNextCharacter(uint8_t c, psMessage_t *rxmsg, status_t *status) {
    
    status->parse_result = PARSE_OK;
    
    switch (status->parse_state) {
        case PARSE_STATE_UNINIT:
            status->packet_rx_drop_count = 0;
            status->packet_rx_success_count = 0;
            status->parse_error = 0;
            status->current_rx_seq = -1;
            status->parse_state = PARSE_STATE_IDLE;
        case PARSE_STATE_IDLE:
            if (c == STX_CHAR) {
                status->parse_state = PARSE_STATE_GOT_STX;
                start_checksum(status);
            }
            break;
        case PARSE_STATE_GOT_STX:
            rxmsg->header.length = c;
            update_checksum(status, c);
            if (status->noLength2) {
                if (status->noSeq) {
                    status->parse_state = PARSE_STATE_GOT_SEQ;
                } else {
                    status->parse_state = PARSE_STATE_GOT_LENGTH2;
                }
            } else {
                status->parse_state = PARSE_STATE_GOT_LENGTH1;
            }
            break;
        case PARSE_STATE_GOT_LENGTH1:
        {
            uint8_t l = rxmsg->header.length;
            uint8_t lc = l | c;
            if (lc == 0xff) {
                update_checksum(status, c);
                if (status->noSeq) {
                    status->parse_state = PARSE_STATE_GOT_SEQ;
                } else {
                    status->parse_state = PARSE_STATE_GOT_LENGTH2;
                }
            } else {
               DEBUGPRINT("Parse: Parse error. Mis-matched length bytes. Got %i and %i.\n", c, l);
               status->parse_result = PARSE_LENGTH_MISMATCH;
               status->parse_error++;
               status->parse_state = PARSE_STATE_IDLE;
            }
        }
            break;
        case PARSE_STATE_GOT_LENGTH2:
        	update_checksum(status, c);
        	status->current_rx_seq = (status->current_rx_seq+1) & 0xff;
        	if (c != status->current_rx_seq) {
                DEBUGPRINT("Parse: Sequence error. Got %i, expected %i.\n", c, status->current_rx_seq);
               status->parse_result = PARSE_SEQUENCE_ERROR;
                status->packet_rx_drop_count++;
        		status->current_rx_seq = c;
        	}
        	status->parse_state = PARSE_STATE_GOT_SEQ;
        	break;
        case PARSE_STATE_GOT_SEQ:
            rxmsg->header.source = c;
            update_checksum(status, c);
            status->parse_state = PARSE_STATE_GOT_SRCID;
            break;
        case PARSE_STATE_GOT_SRCID:
            rxmsg->header.messageType = c;
            update_checksum(status, c);
            status->packet_idx = 0;
            if (c >= PS_MSG_COUNT) {
                DEBUGPRINT("Parse: Parse error. Bad message type: %u\n", c); 
                status->parse_result = PARSE_BAD_MESSAGE_TYPE;
                status->parse_error++;
                if (c == STX_CHAR) {
                    status->parse_state = PARSE_STATE_GOT_STX;
                    rxmsg->header.length = 0;
                    start_checksum(status);
                } else status->parse_state = PARSE_STATE_IDLE;
            } else if ((c < PS_MSG_COUNT) &&
                    (psMessageFormatLengths[psMsgFormats[(int) c]] >= 0 &&
                        psMessageFormatLengths[psMsgFormats[(int) c]] != rxmsg->header.length)) {
                DEBUGPRINT("Parse: Parse error. Message type %u. Expected length %u, got %u.\n", c, psMessageFormatLengths[psMsgFormats[(int) c]], rxmsg->header.length);
                status->parse_result = PARSE_BAD_MESSAGE_LENGTH;
                status->parse_error++;
                status->parse_state = PARSE_STATE_IDLE;

            } else {
            	if (status->noTopic)
            	{
            		if (rxmsg->header.length == 0) {
            			status->parse_result = PARSED_MESSAGE;
            			status->packet_rx_success_count++;
            			status->parse_state = PARSE_STATE_IDLE;
            		} else {
            			status->parse_state = PARSE_STATE_GOT_TOPICID;
            		}
            	}
            	else
            	{
            		status->parse_state = PARSE_STATE_GOT_MSGID;
            	}
            }
            break;
        case PARSE_STATE_GOT_MSGID:
        	rxmsg->header.messageTopic = c;
        	update_checksum(status, c);
        	status->packet_idx = 0;
        	if (rxmsg->header.length == 0) {
        		status->parse_result = PARSED_MESSAGE;
        		status->packet_rx_success_count++;
        		status->parse_state = PARSE_STATE_IDLE;
        	} else {
        		status->parse_state = PARSE_STATE_GOT_TOPICID;
        	}
        	break;
        case PARSE_STATE_GOT_TOPICID:
            rxmsg->packet[status->packet_idx++] = c;
            update_checksum(status, c);
            if (status->packet_idx >= rxmsg->header.length) {
                if (status->noCRC)
                {
                    status->parse_result = PARSED_MESSAGE;
                    status->packet_rx_success_count++;
                    status->parse_state = PARSE_STATE_IDLE;
                }
                else
                {
                    status->parse_state = PARSE_STATE_GOT_PAYLOAD;
                }
            }
        	break;
        case PARSE_STATE_GOT_PAYLOAD:
            // Checking checksum byte
            if (c != (status->checksum & 0xFF)) {
                DEBUGPRINT("Parse: Parse error. Expected checksum %u, got %u.\n", (status->checksum & 0xff), c);
                status->parse_result = PARSE_CHECKSUM;
                status->parse_error++;
                status->parse_state = PARSE_STATE_IDLE;
                if (c == STX_CHAR) {
                    status->parse_state = PARSE_STATE_GOT_STX;
                    rxmsg->header.length = 0;
                    start_checksum(status);
                }
            } else {
                status->parse_result = PARSED_MESSAGE;
                status->packet_rx_success_count++;
                status->parse_state = PARSE_STATE_IDLE;
            }
        default:
            break;
        }
    return status->parse_result;
}

void ResetParseStatus(status_t *status) {
    status->parse_state 			= PARSE_STATE_UNINIT;
    status->packet_rx_success_count	= 0;	///< Received packets
    status->packet_rx_drop_count	= 0;	///< Number of packet drops
    status->checksum				= 0;	///< Running checksum
    status->parse_result			= 0;	///< Number of received messages
    status->parse_error				= 0;	///< Number of parse errors
    status->packet_idx				= 0;	///< Index in current packet
    status->current_rx_seq			= 0;	///< Sequence number of last packet received
}
