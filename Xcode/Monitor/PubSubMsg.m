//
//  PubSubMsg.m
//  FIDO
//
//  Created by Martin Lane-Smith on 12/5/13.
//  Copyright (c) 2013 Martin Lane-Smith. All rights reserved.
//

#import "PubSubMsg.h"
#import "PubSubData.h"
#import "BLE.h"
#import "AppDelegate.h"
#import "BLEinterface.h"

#import "Helpers.h"

char *subsystemNames[] = SUBSYSTEM_NAMES;

#define messagemacro(m,q,t,f,l) f,
int psMsgFormats[PS_MSG_COUNT] = {
#include "Messages/MessageList.h"
};
#undef messagemacro

#define messagemacro(m,q,t,f,l) l,
char *psLongMsgNames[PS_MSG_COUNT] = {
#include "Messages/MessageList.h"
};
#undef messagemacro

#define messagemacro(m,q,t,f,l) t,
int psDefaultTopics[PS_MSG_COUNT] = {
#include "Messages/MessageList.h"
};
#undef messagemacro

#define formatmacro(e,t,v,s) s,
int psMessageFormatLengths[PS_FORMAT_COUNT] = {
#include "Messages/MsgFormatList.h"
};
#undef formatmacro


@interface PubSubMsg (){
    char encodedMessage[MAX_ENCODED_MESSAGE];
    int encodedLen;
    size_t bytesToSend;
    char *next;
}

@end

@implementation PubSubMsg

+ (bool) sendMessage:(psMessage_t *) msg {
    PubSubMsg *message = [[PubSubMsg alloc] initWithMsg: msg];
    if (message) {
        return [message sendMessage];
    }
    else{
        return NO;
    }
}

+ (bool) sendSimpleMessage: (int) msgType {
    psMessage_t message;
    NSAssert1(msgType < PS_MSG_COUNT,@"Bad Message # %i", msgType);
    NSAssert1(psMsgFormats[msgType] == PS_NO_PAYLOAD, @"%s needs payload", psLongMsgNames[msgType]);
    
    message.header.messageType = msgType;
    
    return [PubSubMsg sendMessage: &message];
}

+ (PubSubMsg*) keepAlive
{
    psMessage_t message;
    message.header.messageType = KEEPALIVE;
    message.header.messageTopic = PS_UNDEFINED_TOPIC;
    message.header.length = 0;
    message.header.source = 0;
    return [[PubSubMsg alloc] initWithMsg:&message];
}

//incoming
- (PubSubMsg*) initWithData:(char *) data
{
    
    if (textToMsg(data, &_msg) < 0) {
        return nil;
    }
    if (_msg.header.messageType >= PS_MSG_COUNT) return nil;
    
    
    self = [super init];
    
    NSString *source = [(SubsystemViewController*)[[MasterViewController getMasterViewController].sourceCodes objectForKey:[NSNumber numberWithInt: _msg.header.source]] name];
    
    if (!source)
    {
        source = @"???";
    }
    
    return self;
}
- (PubSubMsg*) initWithMsg: (psMessage_t *) msg {
    self = [super init];
    
    memcpy(&_msg, msg, sizeof(psMessage_t));
  
    return self;
}


- (bool) sendMessage
{
    
    AdjustMessageLength(&_msg);
    _msg.header.messageTopic = psDefaultTopics[_msg.header.messageType];
    
    encodedLen = msgToText(&_msg, encodedMessage, MAX_ENCODED_MESSAGE);
    
    //prep for BLE
    if (encodedLen < 0) {
        return false;
    }
    next = encodedMessage;
    
    AppDelegate* delegate = (AppDelegate* )[[UIApplication sharedApplication] delegate];
    
    [delegate sendMessage: self];
    
    return true;
}

- (bool) sendPart
{
    if (encodedLen > MAX_BLE_MESSAGE){
        bytesToSend = MAX_BLE_MESSAGE;
        encodedLen -= bytesToSend;
    }
    else
    {
        bytesToSend = encodedLen;
        encodedLen = 0;
    }
    
    NSData *dataToSend = [NSData dataWithBytes: next length: bytesToSend];
    
    if (dataToSend)
        [[BLEinterface getSharedBLEinterface] sendBLEMesage: dataToSend];
    
    next += bytesToSend;
    
    if (encodedLen)
    {
        return true;        //more to send
    }
    else{
        return false;       //done
    }
}

- (void) copyMessage: (psMessage_t *) message
{
    memcpy(message, &_msg, sizeof(psMessage_t));
}

@end
