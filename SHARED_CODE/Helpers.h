/* 
 * File:   Helpers.h
 * Author: martin
 *
 * Helper functions for Messages
 *
 */
 
#ifndef _HELPERS_H_
#define _HELPERS_H_

#include "Messages.h"

#ifdef __cplusplus
extern "C" {
#endif

//Base64 encoding
int Base64encode_len(int len);
int Base64encode(char * coded_dst, const char *plain_src,int len_plain_src);

int Base64decode_len(const char * coded_src);
int Base64decode(char * plain_dst, const char *coded_src);

//Struct to serial conversion - returns encoded length
int msgToText(psMessage_t *msg, char * textBuffer, int length);

//text to serial - returns -1 for error, 0 for success
int textToMsg(const char * text, psMessage_t *msg) ;

#define MAX_ENCODED_MESSAGE (sizeof(psMessage_t) * 2)

//Adjust the header.msgLength field for variable length messages
void AdjustMessageLength (psMessage_t *_msg);

#ifdef __cplusplus
}
#endif

#endif //_HELPERS_H_
