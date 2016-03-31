//
//  MapViewController.m
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 11/27/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "MapViewController.h"
#import "WPTableViewController.h"
#import "Helpers.h"
#import "Mapping.h"
#import "Waypoints.h"

#define PIXEL_TO_CM 			28.0f	 /* 30.4805793f		pixels to cm */

@interface MapViewController () {
    UIScrollView *scrollView;
    UIImageView *mapImageView;
    WPEditController *editController;
    
    UIPanGestureRecognizer *panGesture;
    UITapGestureRecognizer *doubleTapGesture;
    
    CGPoint startPan, startOrigin;
    
    psPosePayload_t         poseReport;
    psPositionPayload_t     positionReport;
    ps3FloatPayload_t       compassReport;
    
    bool connected;
    
    UIImage *define_location, *dog, *red_dot;

}



@property CLLocationManager *locationManager;

@property UIImageView       *myLocationView;
@property UIImageView       *fidoLocationView;
@property NSMutableArray    *waypointViews;
@property UILabel           *lblLocationGPS;

- (void) redrawWaypoints;

@end

@implementation MapViewController

- (MapViewController*) init
{
    if (self = [super init])
    {
        self.view = scrollView = [[UIScrollView alloc] init];
        mapImageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"NewMap.png"]];
        [scrollView addSubview:mapImageView];
        
        self.lblLocationGPS = [[UILabel alloc] initWithFrame:CGRectMake(10,20,500,30)];
        _lblLocationGPS.text = @"";
        [scrollView addSubview:_lblLocationGPS];
        
        self.locationManager = [[CLLocationManager alloc] init];
        _locationManager.desiredAccuracy = kCLLocationAccuracyBest;
        _locationManager.distanceFilter = kCLDistanceFilterNone;
        _locationManager.delegate = self;
        self.location = nil;
        
        if ([self.locationManager respondsToSelector:@selector(requestWhenInUseAuthorization)]) {
            [self.locationManager requestWhenInUseAuthorization];
        }
        
        [_locationManager startUpdatingLocation];

        self.myLocationView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"define_location.png"]];
        self.myLocationView.hidden = true;
        self.fidoLocationView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"dog.png"]];
        self.fidoLocationView.hidden = true;
        
        //add gestures
        panGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panAction)];
        [self.view addGestureRecognizer:panGesture];
        
        doubleTapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(markButton)];
        doubleTapGesture.numberOfTapsRequired = 2;
        [self.view addGestureRecognizer:doubleTapGesture];
        
        define_location = [UIImage imageNamed:@"define_location.png"];
        dog = [UIImage imageNamed:@"dog.png"];
        red_dot = [UIImage imageNamed:@"red_dot.png"];
        
        [self mapAction:LOAD_WAYPOINTS];
     }
    return self;
}

- (void)viewWillAppear:(BOOL)animated {
    
    [self redrawWaypoints];

    CGRect scroll = scrollView.bounds;
    CGRect map = mapImageView.frame;
    
    map.origin.x = (scroll.size.width - map.size.width) / 2;
    if (scroll.size.height > map.size.height)
    {
        map.origin.y = (scroll.size.height - map.size.height) / 2;
    }
    else{
        map.origin.y = 0;
    }
    mapImageView.frame = map;
    
    [super viewWillAppear:animated];
}

- (void) viewDidAppear: (BOOL)animated
{
    CGRect scroll = scrollView.bounds;
    CGRect map = mapImageView.frame;
    
    map.origin.x = (scroll.size.width - map.size.width) / 2;
    if (scroll.size.height > map.size.height)
    {
        map.origin.y = (scroll.size.height - map.size.height) / 2;
    }
    else{
        map.origin.y = 0;
    }
   mapImageView.frame = map;
    
    self.navigationController.navigationBar.topItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"map_marker-32.png"]
                                                                                                          style:UIBarButtonItemStylePlain
                                                                                                         target:self
                                                                                                         action:@selector(markButton)];
    
    [super viewDidAppear:animated];
}

- (void) panAction
{
    switch (panGesture.state)
    {
        case UIGestureRecognizerStateBegan:
        {
            startPan = [panGesture locationInView:self.view];
            startOrigin = mapImageView.frame.origin;
        }
            break;
        case UIGestureRecognizerStateChanged:
        {
            CGPoint changedPan = [panGesture locationInView:self.view];
            CGRect map = mapImageView.frame;
            CGRect scroll = scrollView.bounds;
            
            map.origin.x = startOrigin.x + changedPan.x - startPan.x;
            map.origin.y = startOrigin.y + changedPan.y - startPan.y;
            
            if (scroll.size.width < map.size.width)
            {
                if (map.origin.x + map.size.width < scroll.size.width) map.origin.x =  scroll.size.width - map.size.width;
                if (map.origin.x > 0) map.origin.x = 0;
            }
            else{
                map.origin.x = (scroll.size.width - map.size.width) / 2;
            
            }
            
            if (scroll.size.height < map.size.height)
            {
                if (map.origin.y + map.size.height < scroll.size.height) map.origin.y =  scroll.size.height - map.size.height;
                if (map.origin.y > 0) map.origin.y = 0;
            }
            else{
                map.origin.y = (scroll.size.height - map.size.height) / 2;
            }
            mapImageView.frame = map;
        }
            break;
        default:
            break;
    }
}

- (void) positionSubView: (UIImageView *) vw at: (CGPoint) point
{
    //input is in Fido cm frame
    
    float x = point.x / PIXEL_TO_CM;
    float y = point.y / PIXEL_TO_CM;
    
//    NSLog(@"Self: X= %f, Y= %f", x, y);
    
    if (x > 0 && x < 512 & y > 0 && y < 512)
    {
        CGRect frame = vw.frame;
        frame.origin.x = x - frame.size.width / 2;
        frame.origin.y = y - frame.size.height / 2;
        
        vw.frame = frame;
    }
}

- (void) positionSubView: (UIImageView *) vw with: (CLLocation *) loc
{
    float lat = loc.coordinate.latitude;
    float lon = loc.coordinate.longitude;
    
    CGPoint newPoint;
    newPoint.x = LongitudeToEasting(lon);
    newPoint.y = LatitudeToNorthing(lat);
    
    [self positionSubView:vw at: newPoint];
    vw.hidden = false;
}

- (void) redrawWaypoints
{
    UIImageView *waypoint;
    
    for (UIView *v in mapImageView.subviews)
    {
        [v removeFromSuperview];
    }
    
    [mapImageView addSubview: _myLocationView];
    [mapImageView addSubview: _fidoLocationView];
    
    Waypoint_struct *current = waypointListStart;

    while (current != NULL)
    {
        waypoint = [[UIImageView alloc] initWithImage: red_dot];
        [mapImageView addSubview: waypoint];
        
        NSLog(@"Waypoint:%s", current->waypointName);
        
        CLLocation *loc = [[CLLocation alloc] initWithLatitude:current->position.latitude longitude:current->position.longitude];
        [self positionSubView:waypoint with:loc];
        current = current->next;
    }
}

- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation
{
    NSTimeInterval locationAge = -[newLocation.timestamp timeIntervalSinceNow];
    if (locationAge > 5.0) {
        return;
    }
    
    // test that the horizontal accuracy does not indicate an invalid measurement
    if (newLocation.horizontalAccuracy < 0) {
        return;
    }
    
    _lblLocationGPS.text = [NSString stringWithFormat:@"%fN, %fE (accuracy %.1f)",
                         newLocation.coordinate.latitude,
                         newLocation.coordinate.longitude,
                            newLocation.horizontalAccuracy];
    
    if (newLocation.horizontalAccuracy > 10.0f)
        return;
    
    self.location = newLocation;

    [self positionSubView: _myLocationView with: newLocation];
}
- (void)locationManager:(CLLocationManager *)manager
didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
    NSLog(@"Location authorization: %i", status);
}
- (void)locationManager:(CLLocationManager *)manager
       didFailWithError:(NSError *)error
{
    NSLog(@"Location error: %@", error);
}

- (void) mapAction: (MapAction_enum) action
{
    switch (action)
    {
        case MAP_MARK:
            
            NSLog(@"Mark");
            
           if (_location && _location.horizontalAccuracy < 10)
            {
                editController = [[WPEditController alloc] initWithNibName:@"WPEditController" bundle:[NSBundle mainBundle]];
                editController.delegate = self;
                editController.modalTransitionStyle = UIModalTransitionStyleCoverVertical;
                editController.modalPresentationStyle = UIModalPresentationCurrentContext;
                
                [self presentViewController:editController
                                   animated:YES
                                 completion:nil];
                
                [editController initPanel:  @"New Waypoint"  wpName: @"unnamed" location: _location];
            }
            break;
            
        case  SAVE_WAYPOINTS:
        {
            NSFileManager *fm = [NSFileManager defaultManager];
            NSURL *fileURL = [fm URLForDirectory:NSDocumentDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nil];
            self.waypointFilePath = [NSString stringWithFormat:@"%@/waypoints.xml", fileURL.path];

            if (SaveWaypointDatabase([_waypointFilePath UTF8String]) == 0)
            {
                NSLog(@"Saved waypoints to %@", _waypointFilePath);
            }
            else{
                NSLog(@"Failed to save waypoints to %@", _waypointFilePath);
            }
        }
            break;
            
        case LOAD_WAYPOINTS:
        {
            NSFileManager *fm = [NSFileManager defaultManager];
            NSURL *fileURL = [fm URLForDirectory:NSDocumentDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nil];
            self.waypointFilePath = [NSString stringWithFormat:@"%@/waypoints.xml", fileURL.path];
            
            if (![fm isReadableFileAtPath:_waypointFilePath])
            {
                _waypointFilePath = [[NSBundle mainBundle] pathForResource:@"waypoints" ofType:@"xml"];
            }
            InitWaypointDB();
            if (LoadWaypointDatabase([_waypointFilePath UTF8String]) == 0)
            {
                NSLog(@"Loaded waypoints from %@", _waypointFilePath);
            }
            else{
                NSLog(@"Failed to load waypoints from %@", _waypointFilePath);
            }
        }
            break;
            
        default:
            break;
    }

}

- (void) markButton
{
    [self mapAction:MAP_MARK];
}

//WP Edit Panel finsihed with name of new WP
-(void) waypointName: (NSString*)name cancelled: (BOOL) cancelled;
{
    if (!cancelled)
    {
        Position_struct wpPosition;
        wpPosition.latitude = _location.coordinate.latitude;
        wpPosition.longitude = _location.coordinate.longitude;
        
        AddWaypoint([name UTF8String],  wpPosition);
        
        WPTableViewController *wp = [WPTableViewController getWPTableViewController];
        [wp addWaypoint: name atLatitude:_location.coordinate.latitude longitude:_location.coordinate.longitude];
        
        [self redrawWaypoints];
    }
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


-(void) didConnect{
    
}
-(void) didDisconnect{
    
}
-(void) didReceiveMessage: (PubSubMsg*) message{
    switch(message.msg.header.messageType){
            
        case POSEREP:
        {
            connected = YES;
            poseReport = message.msg.posePayload;
            
            if (message.msg.posePayload.positionValid)
            {
                CGPoint newPoint;
                newPoint.x = LongitudeToEasting(message.msg.posePayload.position.longitude);
                newPoint.y = LatitudeToNorthing(message.msg.posePayload.position.latitude);
                
                [self positionSubView:_fidoLocationView at: newPoint];
                _fidoLocationView.hidden = false;
            }
            else{
                _fidoLocationView.hidden = true;
            }
            
        }
            break;

        default:
            break;
    }
}

@end
