//
//  StatusViewController.h
//  Monitor
//
//  Created by Martin Lane-Smith on 4/14/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MessageDelegateProtocol.h"

#define TIMEOUT_DURATION    30.0f
#define CONFIG_TIMEOUT      120

@class SubsystemViewController;

@protocol StatusUpdate
@optional
- (void) statusChange: (SubsystemViewController*) ss;
@end

@interface SubsystemViewController : UITableViewController <MessageDelegate>

@property (strong) NSString *name;
@property          bool online;
@property          bool configured;
@property          bool initErrors;

@property (strong) NSMutableDictionary *info;
@property (strong) NSArray *sortedInfoNames;

@property (strong) NSMutableDictionary *settings;
@property (strong) NSArray *sortedSettingNames;

@property (strong) NSMutableDictionary *options;
@property (strong) NSArray *sortedOptionNames;

@property (strong) NSMutableDictionary *stats;

- (SubsystemViewController*) initWithMessage: (PubSubMsg*) message;
- (void) addListener: (id <StatusUpdate>) _listener;

- (void) reconfig;

@end
