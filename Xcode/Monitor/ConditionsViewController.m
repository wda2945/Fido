//
//  ConditionsViewController.m
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 1/13/16.
//  Copyright © 2016 Martin Lane-Smith. All rights reserved.
//

#import "ConditionsViewController.h"

#define MAX_CONDITIONS 128

@interface ConditionsViewController () {
    UITableView *tableView;
    NSMutableDictionary *notifications;
    NSArray *sortedList;
    
    bool condition[CONDITION_COUNT];
    bool valid[CONDITION_COUNT];
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
        
        notifications = [NSMutableDictionary dictionary];

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
        
        condition[i] = valid[i] = false;
    }
}

- (bool) isConditionSet: (Condition_enum) c
{
    return (condition[c] & valid[c]);
}

-(void) didConnect{

}

-(void) didDisconnect{}
-(void) didReceiveMessage: (PubSubMsg*) message
{
    int i;
    switch(message.msg.header.messageType){
        case CONDITIONS:
        {
            for (i=0; i < CONDITION_COUNT; i++)
            {
                int bit 	= i % 64;
                int index 	= i / 64;
                
                NotificationMask_t mask = NOTIFICATION_MASK(bit);
                NotificationMask_t C = message.msg.maskPayload.value[index] & mask;
                NotificationMask_t V = message.msg.maskPayload.valid[index] & mask;
                
                condition[i] |= (V & C);
                condition[i] &= ~(V & ~C);
                valid[i] |= V;
            }
            
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
    
    if (condition[indexPath.row + 1] && valid[indexPath.row + 1])
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
