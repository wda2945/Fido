//
//  BLEinterface.h
//  Monitor
//
//  Created by Martin Lane-Smith on 4/16/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "PubSubMsg.h"

#define MAX_BLE_MESSAGE 16

@interface BLEinterface : NSObject

+ (BLEinterface*) getSharedBLEinterface;

- (BLEinterface*) init;

- (void) sendBLEMesage: (NSData*) message;

@end
