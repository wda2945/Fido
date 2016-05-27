//
//  DataViewController.m
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 1/25/15.
//  Copyright (c) 2015 Martin Lane-Smith. All rights reserved.
//

#import "DataViewController.h"
#import "MasterViewController.h"
#import "SubsystemViewController.h"

@interface DataViewController ()
{
    UITableView *tableView;
}
@end

@implementation DataViewController

- (DataViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;
    }

    return self;
}


- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
    
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView{
    return [MasterViewController getMasterViewController].subsystems.count;
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section{
    return [[MasterViewController getMasterViewController].subsystems.allKeys objectAtIndex:section];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    SubsystemViewController *ss =
    [[MasterViewController getMasterViewController].subsystems objectForKey:
     [[MasterViewController getMasterViewController].subsystems.allKeys objectAtIndex:section]];
    return ss.info.count;
    return 0;
}
- (UITableViewCell *)tableView:(UITableView *)_tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath{
    SubsystemViewController *ss =
    [[MasterViewController getMasterViewController].subsystems objectForKey:
     [[MasterViewController getMasterViewController].subsystems.allKeys objectAtIndex:indexPath.section]];
    
    NSString *key = [ss.info.allKeys objectAtIndex:indexPath.row];
    NSMutableDictionary *dict = [ss.info objectForKey:key];
    
    UITableViewCell *cell = [_tableView dequeueReusableCellWithIdentifier:@"DataCell"];
    
    if (!cell)
    {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"DataCell"];
    }
    cell.textLabel.text = [dict objectForKey:@"name"];
    cell.detailTextLabel.text = [(NSNumber*)[dict objectForKey:@"value"] stringValue];
    return cell;
}
- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    return nil;
}

-(void) didConnect{
    [tableView reloadData];
}
-(void) didDisconnect{
    [tableView reloadData];
}
-(void) didReceiveMessage: (PubSubMsg*) message{
    switch (message.msg.header.messageType)
    {
        case SS_ONLINE:
        case FLOAT_DATA:
        case INT_DATA:
            [tableView reloadData];
            break;
        default:
            break;
    }
}

@end
