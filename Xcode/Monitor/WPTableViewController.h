//
//  WPTableViewController.h
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 11/27/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>
#import "MessageDelegateProtocol.h"


@interface WPTableViewController : UITableViewController < MessageDelegate>

@property (strong)      NSMutableArray *waypoints;

+ (WPTableViewController*) getWPTableViewController;

- (void) addWaypoint: (NSString*)name atLatitude: (double) _latitude longitude: (double) _longitude;

@end
