//
//  SystemViewController.m
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 6/27/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "SystemViewController.h"
#import "PowerViewController.h"

@interface SystemViewController () {
    NSString *powerStateStr;               //state reported by robot
    
    NSString *currentActivity;               //behavior reported by overmind
    BehaviorStatus_enum activityStatus;
    NSString *lastCallFail;
    NSString *lastCallFailReason;
    
    bool connected;                     //BLE connected
    
    BatteryStatus_enum batteryStatus;   //battery state reported by robot
    float volts;                        //battery voltage
    int percentage;
    float amphours;
    
    UITableView             *tableView;
    psPosePayload_t         poseReport;
    
    NSMutableDictionary *notifications;
    NSArray *sortedList;
}
@property NSString *powerState;
@property (strong) NSTimer *onlineTimeout;
#define TIMEOUT_DURATION 20

@end

@implementation SystemViewController


char *commandNames[] = USER_COMMAND_NAMES;
char *activityStatusNames[] = BEHAVIOR_STATUS_NAMES;

- (SystemViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;

        connected = NO;
        self.powerState = @"unknown";
        powerStateStr   = @"unknown";
        currentActivity = @"unknown";
        lastCallFail       = @"";
        lastCallFailReason = @"";
        activityStatus  = BEHAVIOR_INVALID;
        
        batteryStatus = 0;
        volts = 0.0;
        percentage = 0.0;
        amphours = 0.0;
        
        poseReport.orientation.heading  = 0;
        poseReport.orientation.pitch    = 0;
        poseReport.orientation.roll     = 0;
        poseReport.position.latitude      = 0;
        poseReport.position.longitude       = 0;
        
        notifications = [NSMutableDictionary dictionary];
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
-(void) didConnect{
    connected = YES;
}
-(void) didDisconnect{
    connected = NO;
    self.powerState = POWER_STATE_UNKNOWN;
    powerStateStr = @"unknown";
    currentActivity = @"unknown";
    batteryStatus = 0;
    [(UITableView*)self.view reloadData];
}
//no messages at all - assume offline
- (void) Offline: (NSTimer*) timer{
    self.powerState = POWER_STATE_UNKNOWN;
    powerStateStr = @"unknown";
    currentActivity = @"unknown";
    batteryStatus = 0;
    [(UITableView*)self.view reloadData];
}
- (void) restartTimeout {
    if (self.onlineTimeout) {
        [self.onlineTimeout invalidate];
    }
    self.onlineTimeout =     [NSTimer scheduledTimerWithTimeInterval:TIMEOUT_DURATION
                                                              target:self
                                                            selector:@selector(Offline:)
                                                            userInfo:nil
                                                             repeats:NO];
}

-(void) didReceiveMessage: (PubSubMsg*) message{
    char behavior[PS_SHORT_NAME_LENGTH];
    char lastFail[PS_SHORT_NAME_LENGTH];
    char reason[PS_SHORT_NAME_LENGTH];
   
    [self restartTimeout];
    
    switch(message.msg.header.messageType){
        case POSEREP:
            connected = YES;
            poseReport = message.msg.posePayload;
            [(UITableView*)self.view reloadData];
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
            break;
        case BATTERY:
            volts = message.msg.batteryPayload.volts / 10.0;
            batteryStatus = message.msg.batteryPayload.status;
            amphours = message.msg.batteryPayload.ampHours / 10.0;
            percentage = message.msg.batteryPayload.percentage;
            [(UITableView*)self.view reloadData];
            break;
        case POWER_STATE:
        {
            int PS = message.msg.bytePayload.value;
            
            if (PS >= POWER_STATE_COUNT) PS = 0;
            
            self.powerState = [NSString stringWithFormat:@"%s", powerstateNames[PS]];
            [(UITableView*)self.view reloadData];
        }
            break;
         default:
            break;
    }
}

- (void)didReceiveMemoryWarning
{
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
    return 3;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    switch(section)
    {
        case 0:             //battery
            return 2;
            break;
        case 1:             //robot state
            if ([lastCallFailReason length] != 0) return 4;
            else if ([lastCallFail length] != 0) return 3;
            else return 2;
            break;
        case 2:
            return 7;
            break;
        default:
            return 0;
            break;
    }
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    switch(section)
    {
        case 0:             //battery
            return @"Power";
            break;
        case 1:             //reported power state
            return @"Activity";
            break;
        case 2:
            return @"Position & Pose";
            break;
        default:
            return @"";
            break;
    }
    
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    switch(indexPath.section)
    {
            
        case 0:     //battery
        {
            UITableViewCell   *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue2 reuseIdentifier:@"xxx"];
            switch (indexPath.row)
            {
//                case 0:
//                    cell.textLabel.text = @"Voltage";
//                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%.1f V", volts];
//                    break;
//                case 1:
//                    cell.textLabel.text = @"AmpHours";
//                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%.1f Ah", amphours];
//                    break;
//                case 2:
//                    cell.textLabel.text = @"Percentage";
//                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%i%%", percentage];
//                    break;
                default:
                    cell.textLabel.text = @"State";
                    cell.detailTextLabel.text = self.powerState;
                    break;
                case 0:
                    cell.textLabel.text = @"Battery";
                    switch (batteryStatus)
                {
                    case BATTERY_UNKNOWN_STATUS:
                        cell.detailTextLabel.text = @"unknown";
                        break;
                    case BATTERY_NOMINAL_STATUS:
                        cell.detailTextLabel.text = @"Normal";
                        cell.detailTextLabel.textColor = [UIColor greenColor];
                        break;
                    case BATTERY_LOW_STATUS:
                        cell.detailTextLabel.text = @"LOW";
                        cell.detailTextLabel.textColor = [UIColor yellowColor];
                        break;
                    case BATTERY_CRITICAL_STATUS:
                        cell.detailTextLabel.text = @"CRITICAL";
                        cell.detailTextLabel.textColor = [UIColor redColor];
                        break;
                    case BATTERY_SHUTDOWN_STATUS:
                        cell.detailTextLabel.text = @"SHUTDOWN";
                        cell.detailTextLabel.textColor = [UIColor redColor];
                    default:
                        break;
                }
                    break;
            }
            return cell;
        }
            break;
        case 1:     //reported state
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
        case 2:     //pose
        {
            UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue2 reuseIdentifier:@"xxy"];
            
            switch (indexPath.row) {
                case 0:
                    cell.textLabel.text = @"Heading:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%i", poseReport.orientation.heading];
                    break;
                case 1:
                    cell.textLabel.text = @"Pitch:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%i", poseReport.orientation.pitch];
                    break;
                case 2:
                    cell.textLabel.text = @"Roll:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%i", poseReport.orientation.roll];
                    break;
                case 3:
                    cell.textLabel.text = @"IMU Good?";
                    cell.detailTextLabel.text = (poseReport.orientationValid ? @"Yes" : @"No");
                    break;
                case 4:
                    cell.textLabel.text = @"Latitude:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%f", poseReport.position.latitude];
                    break;
                case 5:
                    cell.textLabel.text = @"Longitude:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%f", poseReport.position.longitude];
                    break;
                case 6:
                    cell.textLabel.text = @"GPS Good?";
                    cell.detailTextLabel.text = (poseReport.positionValid ? @"Yes" : @"No");
                    break;
            }
            return cell;
        }
            break;
    }
    
    return nil;
}
- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    return nil;
}

@end
