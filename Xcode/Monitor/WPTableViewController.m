//
//  WPTableViewController.m
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 11/27/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "WPTableViewController.h"
#import "Waypoints.h"

@interface WPTableViewController () {
    UITableView *tableView;
    
    UIImage *define_location, *dog, *flag_red, *flag_yellow, *flag_blue, *flag_green, *location, *street_view;
}

@end

WPTableViewController* wpViewController;

@implementation WPTableViewController

- (WPTableViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;
        wpViewController = self;
        
        define_location = [UIImage imageNamed:@"define_location.png"];
        dog = [UIImage imageNamed:@"dog.png"];
        flag_red = [UIImage imageNamed:@"flag_red.png"];
        flag_yellow  = [UIImage imageNamed:@"flag_yellow.png"];
        flag_blue = [UIImage imageNamed:@"flag_blue.png"];
        flag_green = [UIImage imageNamed:@"flag_green.png"];
        location  = [UIImage imageNamed:@"location.png"];
        street_view = [UIImage imageNamed:@"street_view.png"];
        
        self.waypoints = [NSMutableArray array];
        Waypoint_struct *current = waypointListStart;
        
        while (current != NULL)
        {
            UIImage *icon = flag_yellow;
            
            
            NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:
                                  [[CLLocation alloc] initWithLatitude:current->position.latitude
                                                             longitude:current->position.longitude], @"position",
                                  [NSString stringWithFormat:@"%s", current->waypointName], @"name",
                                  icon, @"icon",
                                  nil];
            
            [self.waypoints addObject:dict];
            
            current = current->next;
        }
    }
    return self;
}

+ (WPTableViewController*) getWPTableViewController
{
    return wpViewController;
}

- (void) addWaypoint: (NSString*)name atLatitude: (double) _latitude longitude: (double) _longitude
{
    
    NSLog(@"New Waypoint %@ at %fN, %fE", name, _latitude, _longitude);
    
    CLLocation *newloc = [[CLLocation alloc] initWithLatitude:_latitude longitude:_longitude];
    
    NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:
                          newloc, @"position",
                          [name copy], @"name",
                          location, @"icon",
                          nil];
    [_waypoints addObject:dict];
   
    [tableView reloadData];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return _waypoints.count;
}

- (UITableViewCell *)tableView:(UITableView *)_tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath{
    
   
    NSMutableDictionary *dict = [_waypoints objectAtIndex:indexPath.row];
    
    UITableViewCell *cell = [_tableView dequeueReusableCellWithIdentifier:@"WPCell"];
    
    if (!cell)
    {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"WPCell"];
    }
    cell.textLabel.text = [dict objectForKey:@"name"];
    
    CLLocation *loc = [dict objectForKey:@"position"];
    cell.detailTextLabel.text =  [NSString stringWithFormat:@"%fN, %fE",
                  loc.coordinate.latitude, loc.coordinate.longitude];
    
    cell.imageView.image = [dict objectForKey:@"icon"];
    
    return cell;
}

-(void) didConnect{
}
-(void) didDisconnect{
}
-(void) didReceiveMessage: (PubSubMsg*) message {    
}

@end
