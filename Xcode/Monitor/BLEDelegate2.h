//
//  BLEDelegate2.h
//  FIDO
//
//  Created by Martin Lane-Smith on 1/1/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#ifndef FIDO_BLEDelegate2_h
#define FIDO_BLEDelegate2_h
#import "PubSubMsg.h"


@protocol BLEDelegate2
@optional
-(void) bleDidConnect;
-(void) bleDidDisconnect;
-(void) bleDidUpdateRSSI:(NSNumber *) rssi;
-(void) didReceiveRobotMsg: (PubSubMsg*) message;
@required
@end

#endif
