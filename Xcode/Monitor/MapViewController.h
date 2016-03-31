//
//  MapViewController.h
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 11/27/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>
#import "WPEditController.h"
#import "MessageDelegateProtocol.h"

typedef enum {
    MAP_MARK, LOAD_WAYPOINTS, SAVE_WAYPOINTS, MAP_ACTION_COUNT
} MapAction_enum;

#define MAP_ACTION_NAMES {"Mark", "Load", "Save"}


@interface MapViewController : UIViewController <MessageDelegate, CLLocationManagerDelegate, WPEditDelegate>

@property CLLocation    *location;  //current device location
@property NSString      *waypointFilePath;

- (void) mapAction: (MapAction_enum) action;


@end
