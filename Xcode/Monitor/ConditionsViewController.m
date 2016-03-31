//
//  ConditionsViewController.m
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 1/13/16.
//  Copyright © 2016 Martin Lane-Smith. All rights reserved.
//

#import "ConditionsViewController.h"

@interface ConditionsViewController () {
    UITableView *tableView;
    NSMutableDictionary *notifications;
    NSArray *sortedList;
    
    NotificationMask_t conditions;
    NotificationMask_t valid;
}

@end

@implementation ConditionsViewController

- (ConditionsViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;
        
//        notifications = [NSMutableDictionary dictionary];
    }
    return self;
}

#define CONDITION(e, n) n,
char *conditionNames[]		= {
#include "Messages/NotificationConditionsList.h"
};
#undef CONDITION

- (void)viewDidLoad {
    [super viewDidLoad];
    
    
    for (int i=1; i< CONDITION_COUNT; i++)
    {
        NSNumber *key = [NSNumber numberWithInt:i];
        
        NSString *name = [NSString stringWithFormat:@"%s", conditionNames[i]];
        [notifications setObject:name forKey:key];
        
    }
}
- (void) clearConditions
{
    conditions = valid = 0;
    [(UITableView*)self.view reloadData];
}
-(void) didConnect{
    conditions = valid = 0;
    [(UITableView*)self.view reloadData];
}

-(void) didDisconnect{}
-(void) didReceiveMessage: (PubSubMsg*) message
{
    switch(message.msg.header.messageType){
        case CONDITIONS:
        {
            NotificationMask_t C = message.msg.eventMaskPayload.value;
            NotificationMask_t V = message.msg.eventMaskPayload.valid;
            
            conditions |= (V & C);
            
            if ((V & ~C))
            {
                conditions &= ~(V & ~C);
            }
            
            valid |= V;
            
            [(UITableView*)self.view reloadData];
        }
            break;
        default:
            break;
    }

}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    return NO;
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return CONDITION_COUNT-1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell   *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"xxw"];
            
    cell.textLabel.text = [NSString stringWithFormat:@"%s", conditionNames[indexPath.row+1]];
    
    if (conditions & valid & NOTIFICATION_MASK((indexPath.row + 1)))
    {
        cell.accessoryType = UITableViewCellAccessoryCheckmark;
    }
    else
    {
        cell.accessoryType = UITableViewCellAccessoryNone;
    }
    
    return cell;
}

@end
