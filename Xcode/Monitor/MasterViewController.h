//
//  MasterViewController.h
//  Monitor
//
//  Created by Martin Lane-Smith on 4/12/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "AppDelegate.h"
#import "MessageDelegateProtocol.h"
#import "CollectionViewController.h"
#import "LogViewController.h"
#import "SensorViewController.h"
#import "StatsViewController.h"
#import "RCViewController.h"
#import "SystemViewController.h"
#import "MapViewController.h"
#import "WPTableViewController.h"
#import "SubsystemViewController.h"
#import "DataViewController.h"
#import "OptionsViewController.h"
#import "SettingsViewController.h"
#import "CommsViewController.h"
#import "BehaviorViewController.h"
#import "PowerViewController.h"
#import "ConditionsViewController.h"
#import "CollectionProtocol.h"

#define PING_INTERVAL 10.0f

@interface MasterViewController : UITableViewController <MessageDelegate, CollectionController, StatusUpdate>

@property (strong, nonatomic) SystemViewController      *systemViewController;
@property (strong, nonatomic) StatsViewController       *statsController;
@property (strong, nonatomic) LogViewController         *logViewController;
@property (strong, nonatomic) SensorViewController      *navController;
@property (strong, nonatomic) RCViewController          *rcController;
@property (strong, nonatomic) SubsystemViewController   *statusController;
@property (strong, nonatomic) MapViewController         *mapController;
@property (strong, nonatomic) WPTableViewController     *wpController;

@property (strong, nonatomic) DataViewController        *dataController;
@property (strong, nonatomic) OptionsViewController     *optionsController;
@property (strong, nonatomic) SettingsViewController    *settingsController;
@property (strong, nonatomic) CommsViewController       *commsViewController;
@property (strong, nonatomic) BehaviorViewController    *behaviorViewController;
@property (strong, nonatomic) PowerViewController       *powerViewController;
@property (strong, nonatomic) ConditionsViewController  *conditionsViewController;

@property (strong, nonatomic) NSMutableDictionary *viewControllers;

@property (strong, nonatomic) UIStoryboard *storyBoard;
@property (strong, nonatomic) UIViewController<CollectionController> *collectionController;

@property (strong, nonatomic) NSMutableDictionary *sourceCodes;     //subsystems by source number
@property (strong, nonatomic) NSMutableDictionary *subsystems;      //subsystems by name

@property (nonatomic) struct timeval pingTime;

+ (MasterViewController*) getMasterViewController;

- (void) changeSubsystemName: (SubsystemViewController*) obj oldName: (NSString*) n;

- (void)applicationDidEnterBackground;

- (void)applicationDidBecomeActive;

- (void) reloadTable;

- (void) didReceiveMessage: (PubSubMsg*) message;

- (void) setConnectionCaption: (NSString*) caption forServer: (int) index connected: (int) conn;

@end
