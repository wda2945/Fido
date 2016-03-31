//
//  WPiPhoneController.h
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 11/27/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>

@protocol WPEditDelegate
@required
-(void) waypointName: (NSString*)name cancelled: (BOOL) cancelled;
@end

@interface WPEditController : UIViewController

@property NSObject<WPEditDelegate> *delegate;

- (void) initPanel: (NSString*) title wpName: (NSString*) name location: (CLLocation*) loc;

@end
