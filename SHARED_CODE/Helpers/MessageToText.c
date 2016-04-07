/*
 * File:   MessageToText.c
 * Author: martin
 *
 * Created on January 27, 2014, 1:28 PM
 */

//conversion utilities
#include <stddef.h>

#include "messages.h"
#include "helpers.h"

//convert a psMessage_t struct into an ASCII string
int msgToText(psMessage_t *msg, char * textBuffer, int length) {
    
    int msgLength = msg->header.length + SIZEOF_HEADER;
    
    if (Base64encode_len(msgLength)+1 > length)
    {
        return -1;
    }
    
    int encodedLen = (Base64encode(textBuffer, (const char *)msg, msgLength));
    textBuffer[encodedLen-1] = '!';
    textBuffer[encodedLen] = '\0';
    return (encodedLen+1);
}

//convert an ASCII string to psMessage_t binary
int textToMsg(const char * text, psMessage_t *msg) {

    int a = Base64decode_len(text);
    int b = sizeof(psMessage_t);
    
    if (a > b) {
        return -1;
    }

    int bytesDecoded = Base64decode((char *) msg, text);
    
    if (bytesDecoded != msg->header.length + SIZEOF_HEADER)
    {
        return -1;
    }
    
    return 0;
}
