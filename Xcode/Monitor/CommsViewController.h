//
//  ProximityViewController.h
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 2/13/15.
//  Copyright (c) 2015 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MessageDelegateProtocol.h"

@interface CommsViewController : UITableViewController <MessageDelegate>

- (void) newConnectionCaption: (NSString*) caption forServer: (int) index connected: (bool) conn;
- (void) newLatencyMeasure: (float) millisecs forServer: (int) index;

- (UIImage*) getConnectionIcon: (int) index;

@end
