//
//  BehaviorController
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 5/4/15.
//  Copyright (c) 2015 Martin Lane-Smith. All rights reserved.
//

#import "BehaviorViewController.h"
#import "MasterViewController.h"
#import "MapViewController.h"

@interface BehaviorViewController () {
    NSString *currentActivity;               //behavior reported by overmind
    BehaviorStatus_enum activityStatus;
    NSString *lastCallFail;
    NSString *lastCallFailReason;
    
    UITableView *tableView;
    
}

@end


@implementation BehaviorViewController

static long sentRow = -1;

- (BehaviorViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;
        
        currentActivity = @"unknown";
        lastCallFail       = @"";
        lastCallFailReason = @"";
        activityStatus  = BEHAVIOR_INVALID;
        
        self.availableScripts = [NSMutableDictionary dictionary];
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    UITableView *table = (UITableView*) self.view;
    table.separatorStyle = UITableViewCellSeparatorStyleNone;
    table.sectionHeaderHeight = 50;
    
}

- (void) clearScripts
{
    currentActivity = @"unknown";
    lastCallFail       = @"";
    lastCallFailReason = @"";
    activityStatus  = BEHAVIOR_INVALID;
    
    self.availableScripts = [NSMutableDictionary dictionary];
    [tableView reloadData];
}
#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    // Return the number of sections.
    return 2;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
    switch(section)
    {
        case 0:
            return @"Activity Status";
            break;
        default:
            return @"Available Behaviors";
            break;
    }
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    // Return the number of rows in the section.
    switch(section)
    {
        case 0:
            if ([lastCallFailReason length] != 0) return 4;
            else if ([lastCallFail length] != 0) return 3;
            else return 2;
            break;
        default:
            return _availableScripts.count;
            break;
    }
}


- (UITableViewCell *)tableView:(UITableView *)tV cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    char *activityStatusNames[BEHAVIOR_STATUS_COUNT] = BEHAVIOR_STATUS_NAMES;
    
    switch (indexPath.section)
    {
        case 0:
        {
            UITableViewCell   *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue2 reuseIdentifier:@"xxy"];
            switch (indexPath.row)
            {
                case 0:
                    cell.textLabel.text = @"Behavior";
                    cell.detailTextLabel.text = currentActivity;
                    break;
                case 1:
                    cell.textLabel.text = @"Status";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%s", activityStatusNames[activityStatus]];
                    break;
                case 2:
                    cell.textLabel.text = @"Failed at";
                    cell.detailTextLabel.text = lastCallFail;
                    break;
                default:
                    cell.textLabel.text = @"Reason";
                    cell.detailTextLabel.text = lastCallFailReason;
                    break;
                    
            }
            return cell;
        }
            break;
        default:
        {
            UITableViewCell *cell = [tV dequeueReusableCellWithIdentifier:@"motionView"];
            
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"motionView"];
            }
            cell.textLabel.text = [_availableScripts.allKeys objectAtIndex:indexPath.row];
            
            if (indexPath.row == sentRow)
            {
                cell.accessoryType = UITableViewCellAccessoryCheckmark;
            }
            else
            {
                cell.accessoryType = UITableViewCellAccessoryNone;
            }
            
            return cell;
        }
            break;
    }
}



- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    sentRow = indexPath.row;
    
    NSString *script = [_availableScripts.allKeys objectAtIndex:indexPath.row];
    psMessage_t msg;
    CLLocation *location = [MasterViewController getMasterViewController].mapController.location;
    
    msg.header.messageType = HEEL_LOCATION;
    msg.positionPayload.latitude = location.coordinate.latitude;
    msg.positionPayload.longitude = location.coordinate.longitude;
    msg.positionPayload.HDOP = location.horizontalAccuracy;
    if (location.horizontalAccuracy <= 10.0)
        msg.positionPayload.gpsStatus = 1;
    else
        msg.positionPayload.gpsStatus = 0;
    [PubSubMsg sendMessage:&msg];
    
    msg.header.messageType = ACTIVATE;
    strncpy(msg.namePayload.name, [script UTF8String], PS_NAME_LENGTH);
    
    [PubSubMsg sendMessage:&msg];
    
    [LogViewController logAppMessage:[NSString stringWithFormat:@"Activate: %@", script]];
    return nil;
}


// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the specified item to be editable.
    return NO;
}

-(void) didConnect{
}
-(void) didDisconnect{
}
-(void) didReceiveMessage: (PubSubMsg*) message{
    char *activityStatusNames[BEHAVIOR_STATUS_COUNT] = BEHAVIOR_STATUS_NAMES;
    char behavior[PS_SHORT_NAME_LENGTH];
    char lastFail[PS_SHORT_NAME_LENGTH];
    char reason[PS_SHORT_NAME_LENGTH];
    
    switch(message.msg.header.messageType)
    {
        case SCRIPT:
        {
            NSString *scriptName    = [NSString stringWithFormat:@"%s", message.msg.namePayload.name];
            NSString *exists        = [_availableScripts objectForKey:scriptName];
            
            if (!exists)    //avoid duplicates
            {
                [_availableScripts setObject:scriptName forKey: scriptName];
                [tableView reloadData];
            }
        }
            break;
        case ACTIVITY:
            strncpy(behavior, message.msg.behaviorStatusPayload.behavior, PS_SHORT_NAME_LENGTH);
            strncpy(lastFail, message.msg.behaviorStatusPayload.lastLuaCallFail, PS_SHORT_NAME_LENGTH);
            strncpy(reason, message.msg.behaviorStatusPayload.lastFailReason, PS_SHORT_NAME_LENGTH);
            behavior[PS_SHORT_NAME_LENGTH-1] = '\0';
            lastFail[PS_SHORT_NAME_LENGTH-1] = '\0';
            reason[PS_SHORT_NAME_LENGTH-1] = '\0';
            
            currentActivity = [NSString stringWithFormat:@"%s", behavior];
            lastCallFail = [NSString stringWithFormat:@"%s", lastFail];
            lastCallFailReason = [NSString stringWithFormat:@"%s", reason];
            activityStatus = message.msg.nameIntPayload.value;
            [(UITableView*)self.view reloadData];

            [LogViewController logAppMessage:[NSString stringWithFormat:@"%@ %s. %@ %@", currentActivity,
                                              activityStatusNames[activityStatus],
                                              lastCallFail, lastCallFailReason]];
            
            break;
        default:
            break;
    }
}


@end
