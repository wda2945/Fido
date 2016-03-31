//
//  OptionsViewController.m
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 2/2/15.
//  Copyright (c) 2015 Martin Lane-Smith. All rights reserved.
//

#import "OptionsViewController.h"
#import "MasterViewController.h"
#import "SubsystemViewController.h"

@interface OptionsViewController ()
{
    UITableView *tableView;
}
@property (strong) NSMutableDictionary *switches;
@property (strong) NSMutableArray      *switchList;

@end

@implementation OptionsViewController

- (OptionsViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;
        
        self.switches    = [NSMutableDictionary dictionary];
        self.switchList  = [NSMutableArray   array];
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
    return ss.options.count;
}

- (UITableViewCell *)tableView:(UITableView *)_tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath{
    
    NSString *ssName = [[MasterViewController getMasterViewController].subsystems.allKeys objectAtIndex:indexPath.section];
    
    SubsystemViewController *ss = [[MasterViewController getMasterViewController].subsystems objectForKey: ssName];
    
    UITableViewCell *cell = [_tableView dequeueReusableCellWithIdentifier:@"OptionsCell"];
    
    if (!cell)
    {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"OptionsCell"];
    }
    if (ss.options.count > indexPath.row)
    {
        NSString *optionName = [ss.sortedOptionNames objectAtIndex: indexPath.row];
        NSMutableDictionary *dict = [ss.options objectForKey:optionName];
        
        cell.textLabel.text = optionName;
        
        UISwitch *sw = [_switches objectForKey:optionName];
        if (!sw) {
            sw = [[UISwitch alloc] init];
            [_switches setObject:sw forKey:optionName];
            sw.tag = _switchList.count;
            [_switchList addObject:optionName];
            [sw addTarget:self action:@selector(swChanged:) forControlEvents:UIControlEventValueChanged];
        }
        sw.on = [(NSNumber*)[dict objectForKey:@"value"] boolValue];
        
        cell.detailTextLabel.text = (sw.on? @"+++ON+++" : @"---OFF---");
        
        cell.accessoryView  = sw;
    }
    cell.imageView.image = [UIImage imageNamed:@"option.png"];
    return cell;
}
- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    return nil;
}

- (void) swChanged: (UISwitch*) sw{
    psMessage_t msg;
    
    NSString *optionName = [_switchList objectAtIndex:sw.tag];
    
    [optionName getCString:msg.optionPayload.name maxLength:PS_NAME_LENGTH encoding:NSASCIIStringEncoding];
    
    msg.header.messageType = SET_OPTION;
    msg.optionPayload.value = (uint8_t) sw.on;
    [PubSubMsg sendMessage:&msg];
    
    [LogViewController logAppMessage:[NSString stringWithFormat:@"%@ = %@", optionName, (sw.on ? @"ON" : @"OFF")]];
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
        case OPTION:
        case CONFIG_DONE:
            [tableView reloadData];
            break;
        default:
            break;
    }
}



@end
