//
//  PubSubMsg.h
//  FIDO
//
//  Created by Martin Lane-Smith on 12/5/13.
//  Copyright (c) 2013 Martin Lane-Smith. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "PubSubData.h"

@interface PubSubMsg : NSObject

@property psMessage_t           msg;

//outgoing
+ (bool) sendMessage:(psMessage_t *) _msg;
+ (bool) sendSimpleMessage: (int) msgType;  //no payload
+ (PubSubMsg*) keepAlive;

//generic
- (PubSubMsg*) initWithMsg: (psMessage_t *) msg;
- (bool) sendMessage;

//incoming
- (PubSubMsg*) initWithData:(char *) _data;

//for tx q
- (bool) sendPart;
- (void) copyMessage: (psMessage_t *) message;


@end
