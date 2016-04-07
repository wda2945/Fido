//
//  PowerViewController.m
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 1/13/16.
//  Copyright © 2016 Martin Lane-Smith. All rights reserved.
//

#import "PowerViewController.h"

@interface PowerViewController () {
    bool connected;                     //BLE connected
    
    BatteryStatus_enum batteryStatus;   //battery state reported by robot
    float volts;                        //battery voltage
    float amps;
    int percentage;
    float amphours;
    
    UITableView *tableView;
}
@property (strong) NSTimer *onlineTimeout;
#define TIMEOUT_DURATION 30
@end

@implementation PowerViewController

char *powerstateNames[] = POWER_STATE_NAMES;

- (void) clearData
{
    self.powerStateStr = @"unknown";
    self.systemStateCommand = COMMAND_ACTIVE;
    self.powerState = POWER_STATE_UNKNOWN;
    batteryStatus = 0;
    volts = 0.0;
    amps = 0.0;
    percentage = 0.0;
    amphours = 0.0;
}
- (PowerViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;
        [self clearData];
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

-(void) didConnect{
    connected = YES;
    
    [self clearData];
    [(UITableView*)self.view reloadData];
}
-(void) didDisconnect{
    connected = NO;
    [self clearData];
    [(UITableView*)self.view reloadData];
}
//no messages at all - assume offline
- (void) Offline: (NSTimer*) timer{
    [self clearData];
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
    
    [self restartTimeout];
    
    switch(message.msg.header.messageType){
        case BATTERY:
            volts = message.msg.batteryPayload.volts / 10.0;
            batteryStatus = message.msg.batteryPayload.status;
            amphours = message.msg.batteryPayload.ampHours / 10.0;
            percentage = message.msg.batteryPayload.percentage;
            amps        = message.msg.batteryPayload.amps / 10.0;
            [(UITableView*)self.view reloadData];
            break;
        case POWER_STATE:
        {
            _powerState = message.msg.bytePayload.value;
            
            if (_powerState >= POWER_STATE_COUNT) _powerState = 0;
            
            self.powerStateStr = [NSString stringWithFormat:@"%s", powerstateNames[_powerState]];
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

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 3;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    switch(section)
    {
        case 0:             //battery
            return 5;
            break;
        case 1:             //robot state
            return 2;
            break;
        case 2:             //command power state
            return COMMAND_COUNT-1;
            break;
        default:
            return 0;
            break;
    }
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
 
    switch(section)
    {
        case 0:             //battery
            return @"Battery";
            break;
        case 1:             //reported power state
            return @"Power State";
            break;
        case 2:             //selected power state
            return @"Power Control";
            break;
        default:
            return @"";
            break;
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static char *commandNames[] = USER_COMMAND_NAMES;
    
    switch(indexPath.section)
    {
            
        case 0:     //battery
        {
            UITableViewCell   *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue2 reuseIdentifier:@"xxx"];
            switch (indexPath.row)
            {
                case 0:
                    cell.textLabel.text = @"Voltage";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%.1f V", volts];
                    break;
                case 1:
                    cell.textLabel.text = @"Amps";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%.1f A", amps];
                    break;
                case 2:
                    cell.textLabel.text = @"AmpHours";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%.1f Ah", amphours];
                    break;
                case 3:
                    cell.textLabel.text = @"Percentage";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%i%%", percentage];
                    break;
                default:
                    cell.textLabel.text = @"Status";
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
                    cell.textLabel.text = @"Requested";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%s", commandNames[_systemStateCommand] ];
                    break;
                default:
                    cell.textLabel.text = @"Reported";
                    cell.detailTextLabel.text = self.powerStateStr;
                    break;
            }
            return cell;
        }
            break;
        case 2:
        {
            UITableViewCell   *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"xxz"];
            
            cell.textLabel.text = [NSString stringWithFormat:@"%s", commandNames[(COMMAND_COUNT - indexPath.row - 1)]];
            
            if ((COMMAND_COUNT - indexPath.row - 1) == self.systemStateCommand)
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
        default:
            return nil;
            break;
    }
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (indexPath.section == 2)
    {
        
        self.systemStateCommand = (UserCommand_enum) (COMMAND_COUNT - indexPath.row - 1);
        [(UITableView*) self.view reloadData];
        
        psMessage_t msg;
        msg.header.messageType = COMMAND;
        msg.bytePayload.value = self.systemStateCommand;
        [PubSubMsg sendMessage:&msg];
    }
    return nil;
}

- (void) newSystemState: (UserCommand_enum) s
{
    self.systemStateCommand = s;
    [(UITableView*) self.view reloadData];
    
    psMessage_t msg;
    msg.header.messageType = COMMAND;
    msg.bytePayload.value = self.systemStateCommand;
    [PubSubMsg sendMessage:&msg];
}

@end
