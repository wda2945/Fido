//
//  BLEinterface.m
//  Monitor
//
//  Created by Martin Lane-Smith on 4/16/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "BLEinterface.h"
#import "AppDelegate.h"

@interface BLEinterface () {
    NSTimer         *gapTimer;
}
@property (strong) NSMutableArray  *messageQueue;

- (void) sendMessage;

@end

static BLEinterface *sharedBLEinterface;

@implementation BLEinterface

+ (BLEinterface*) getSharedBLEinterface
{
    if (!sharedBLEinterface)
    {
        sharedBLEinterface = [[BLEinterface alloc] init];
    }
    return sharedBLEinterface;
}

- (BLEinterface*) init
{
    if (self  = [super init])
    {
        self.messageQueue = [NSMutableArray array];
        gapTimer = nil;
        sharedBLEinterface = self;
    }
    return self;
}

- (void) sendBLEMesage: (NSData*) message
{
    if ([message length] <= MAX_BLE_MESSAGE)
    {
        [_messageQueue addObject:message];
    }
    else
    {
        NSLog(@"Bad BLE Message");
    }
    
    if (!gapTimer)
    {
        [self sendMessage];
    }
}

- (void) sendMessage
{
    NSData *nextMessage = [_messageQueue objectAtIndex: 0];
    if (nextMessage) {
        
        if ([nextMessage length] <= MAX_BLE_MESSAGE)
        {
            [[(AppDelegate*)[[UIApplication sharedApplication] delegate] BLEobject] write:nextMessage];
        }
        else
        {
            NSLog(@"Bad BLE Message");
        }
        [_messageQueue removeObjectAtIndex:0];
        
        if (_messageQueue.count > 0)
        {
            gapTimer = [NSTimer timerWithTimeInterval: 0.25f target:self selector:@selector(sendMessage) userInfo:nil repeats:NO];
            if (!gapTimer)
            {
                NSLog(@"No Timer");
            }
        }
        else{
            gapTimer = nil;
        }
    }else{
        gapTimer = nil;
    }
}

@end
