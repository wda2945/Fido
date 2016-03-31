//
//  CollectionViewController.m
//  Monitor
//
//  Created by Martin Lane-Smith on 4/13/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "CollectionViewController.h"
#import "MasterViewController.h"


@interface CollectionViewController () {
    
    UIViewController *currentController;
    
    CGRect startFrame;
}

@end

@implementation CollectionViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    self.definesPresentationContext = YES;
    
    [self addChildViewController:[MasterViewController getMasterViewController].logViewController];
    [self addChildViewController:[MasterViewController getMasterViewController].statsController];
    [self addChildViewController:[MasterViewController getMasterViewController].navController];
    [self addChildViewController:[MasterViewController getMasterViewController].rcController];
    [self addChildViewController:[MasterViewController getMasterViewController].systemViewController];
    [self addChildViewController:[MasterViewController getMasterViewController].mapController];
    [self addChildViewController:[MasterViewController getMasterViewController].wpController];

    [self addChildViewController:[MasterViewController getMasterViewController].dataController];
    [self addChildViewController:[MasterViewController getMasterViewController].optionsController];
    [self addChildViewController:[MasterViewController getMasterViewController].settingsController];
    [self addChildViewController:[MasterViewController getMasterViewController].commsViewController];
    [self addChildViewController:[MasterViewController getMasterViewController].behaviorViewController];
    [self addChildViewController:[MasterViewController getMasterViewController].powerViewController];
    [self addChildViewController:[MasterViewController getMasterViewController].conditionsViewController];
    startFrame = self.view.bounds;
    startFrame.origin.x += startFrame.size.width;
    
    [MasterViewController getMasterViewController].systemViewController.view.frame = self.view.bounds;
    [self.view addSubview: [MasterViewController getMasterViewController].systemViewController.view];
    [[MasterViewController getMasterViewController].systemViewController didMoveToParentViewController:self];
    currentController = [MasterViewController getMasterViewController].systemViewController;
    
}
- (void) addSubsystem: (SubsystemViewController*) sub
{
    [self addChildViewController:sub];
}
- (bool) presentView: (NSString*) name
{
    UIViewController *controller = [[MasterViewController getMasterViewController].viewControllers objectForKey:name];
    if (controller && controller != currentController) {        
        
        controller.title = name;
        controller.view.frame = startFrame;
       
        [self transitionFromViewController: currentController toViewController: controller 
                                  duration: 0.25 options:0
                                animations:^{
                                    controller.view.frame = self.view.bounds;
                                }
                                completion:nil];
        currentController = controller;
        return YES;
    }
    else return NO;
}
-(void) didConnect
{
//    [[MasterViewController getMasterViewController].viewControllers.allValues makeObjectsPerformSelector:@selector( didConnect)];
}
-(void) didDisconnect
{
//    [[MasterViewController getMasterViewController].viewControllers.allValues makeObjectsPerformSelector:@selector( didDisconnect)];
}
-(void) bleDidUpdateRSSI:(NSNumber *) rssi
{
}
-(void) didReceiveMessage: (PubSubMsg*) message
{
    [[MasterViewController getMasterViewController].viewControllers.allValues makeObjectsPerformSelector:@selector( didReceiveMessage:) withObject:message];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Split view

- (void)splitViewController:(UISplitViewController *)svc
    willChangeToDisplayMode:(UISplitViewControllerDisplayMode)displayMode
{
    
}


@end
