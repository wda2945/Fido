//
//  MasterViewController.m
//  Monitor
//
//  Created by Martin Lane-Smith on 4/12/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "MasterViewController.h"
#import "AppDelegate.h"
#include <sys/time.h>

@interface MasterViewController () {
    
    NSTimer *pingTimer;
    int currentPage;
    
    NSString *connectionCaptions[SERVER_COUNT];
    
    bool channelsConnected[SERVER_COUNT];
    
    UIImage *imgConnected;
    UIImage *imgOffline;
}

- (UIImage*) getStatusImage: (NSString*) subsys;

@end

//pages of App - excluding subsystems
enum { SYSTEM_PAGE, POWER_PAGE, CONDITIONS_PAGE, NAV_PAGE, MAP_PAGE, WP_PAGE, STATS_PAGE, COMMS_PAGE, /*OPTIONS_PAGE, SETTINGS_PAGE, DATA_PAGE, */ RC_PAGE, BEHAVIOR_PAGE, LOG_PAGE, PAGE_COUNT};

static MasterViewController *me;

@implementation MasterViewController

+ (MasterViewController*) getMasterViewController
{
    return me;
}
- (void)awakeFromNib
{
    me = self;
    
    imgConnected = [UIImage imageNamed:@"online.png"];
    imgOffline = [UIImage imageNamed:@"offline.png"];
    
    self.subsystems = [NSMutableDictionary dictionary];
    self.sourceCodes = [NSMutableDictionary dictionary];
    
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        self.clearsSelectionOnViewWillAppear = NO;
        self.preferredContentSize = CGSizeMake(320.0, 600.0);
        self.storyBoard = [UIStoryboard storyboardWithName:@"Main_iPad" bundle:nil];
    } else {
        self.storyBoard = [UIStoryboard storyboardWithName:@"Main_iPhone" bundle:nil];
    }
    
    self.rcController           = [_storyBoard instantiateViewControllerWithIdentifier:@"RC"];
    
    self.systemViewController   = [[SystemViewController alloc] init];
    self.logViewController      = [[LogViewController alloc] init];
    self.statsController        = [[StatsViewController alloc] init];
    self.navController          = [[SensorViewController alloc] init];
    self.mapController          = [[MapViewController alloc] init];
    self.wpController           = [[WPTableViewController alloc] init];
    self.dataController         = [[DataViewController alloc] init];
    self.optionsController      = [[OptionsViewController alloc] init];
    self.settingsController     = [[SettingsViewController alloc] init];
    self.commsViewController    = [[CommsViewController alloc] init];
    self.behaviorViewController   = [[BehaviorViewController alloc] init];
    self.powerViewController    = [[PowerViewController alloc] init];
    self.conditionsViewController   = [[ConditionsViewController alloc] init];
    
    self.viewControllers = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                            _systemViewController, @"System",
                            _logViewController, @"SysLog",
                            _statsController, @"Stats",
                            _navController, @"Nav",
                            _rcController,@"RC",
                            _mapController,@"Map",
                            _wpController,@"Markers",
                            _dataController, @"Data",
                            _optionsController, @"Options",
                            _settingsController, @"Settings",
                            _commsViewController, @"Comms",
                            _behaviorViewController, @"Motion",
                            _powerViewController, @"Power",
                            _conditionsViewController, @"Conditions",
                            nil];
    
    currentPage = SYSTEM_PAGE;
    
    for (int i=0; i<SERVER_COUNT; i++)
    {
        if (i == FIDO_XBEE_SERVER)
            [self setConnectionCaption: @"Searching..." forServer:i connected:NO];
        else
            [self setConnectionCaption: @"Disabled" forServer:i connected:NO];
    }
    [super awakeFromNib];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.

    pingTimer = [NSTimer scheduledTimerWithTimeInterval:PING_INTERVAL
                                                 target:self
                                               selector:@selector(ping:)
                                               userInfo:nil
                                                repeats:YES];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void) setConnectionCaption: (NSString*) caption forServer: (int) index connected: (int) conn
{
    NSString *prefix;
    switch(index)
    {
        case FIDO_OVM_SERVER:
            prefix = @"Fido";
            break;
        case FIDO_XBEE_SERVER:
            prefix = @"XBee";
            break;
        default:
            prefix = @"BLE";
            break;
    }
    
    connectionCaptions[index] = [NSString stringWithFormat:@"%@: %@", prefix, caption];
    channelsConnected[index] = conn;
    [(UITableView*)self.view reloadData];
    
    [_commsViewController newConnectionCaption: connectionCaptions[index] forServer: index connected: conn];
    
    NSLog(@"Caption: %@", connectionCaptions[index]);
}


#pragma mark - Table View

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    if (currentPage == LOG_PAGE || currentPage == MAP_PAGE || currentPage == SYSTEM_PAGE)
    {
        return 4;
    }
    else
    {
        return 3;
    }
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
    switch(section){
        case 0:
            return @"";
            break;
        case 2:
            return @"Subsystems";
            break;
        case 1:
            return @"";
            break;
        default:
            return @"";
            break;
    }
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    switch(section){
        case 0:
            return SERVER_COUNT;
            break;
        case 2:
            return _subsystems.count;
            break;
        case 1:
            return PAGE_COUNT;
            break;
        case 3:
            if (currentPage == LOG_PAGE || currentPage == SYSTEM_PAGE)
                return 1;
            else //if (currentPage == MAP_PAGE)
                return MAP_ACTION_COUNT;
            break;
        default:
            return 0;
            break;
    }
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
    return 20.0f;
}
- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
    return 20.0f;
}
- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return 35.0f;
}

static char *mapActionNames[] = MAP_ACTION_NAMES;
static char *subsystemFullNames[] = SUBSYSTEM_FULL_NAMES;

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell;
    switch(indexPath.section){
        case 0:     // online
        {
            cell = [tableView dequeueReusableCellWithIdentifier:@"MasterCell"];
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"MasterCell"];
            }
            
            cell.textLabel.text = connectionCaptions[indexPath.row];
            cell.imageView.image = [_commsViewController getConnectionIcon: (int) indexPath.row];
            
        }
            break;
        case 2:     //Subsystems
        {
            
            cell = [tableView dequeueReusableCellWithIdentifier:@"SubsystemCell"];
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"SubsystemCell"];
            }
            NSString *ssName = [_subsystems.allKeys objectAtIndex:indexPath.row];
            SubsystemViewController *ss = [_subsystems objectForKey:ssName];
            
            cell.textLabel.text = [NSString stringWithFormat:@"%s", subsystemFullNames[ss.sourceCode]];
            cell.imageView.image = [self getStatusImage:ssName];
            
            if (ss.configured)
            {
                cell.accessoryType = UITableViewCellAccessoryCheckmark;
            }
            else
            {
                cell.accessoryType = UITableViewCellAccessoryNone;
            }
        }
            break;
        case 1:     //Menu
        {
            cell = [tableView dequeueReusableCellWithIdentifier:@"DetailCell"];
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"DetailCell"];
            }
            
            switch (indexPath.row){
                case SYSTEM_PAGE:
                    cell.textLabel.text = @"System";
                    cell.imageView.image = [UIImage imageNamed:@"house.png"];
                    break;
                case POWER_PAGE:
                    cell.textLabel.text = @"Power";
                    cell.imageView.image = [UIImage imageNamed:@"battery"];
                    break;
                case CONDITIONS_PAGE:
                    cell.textLabel.text = @"Notifications";
                    cell.imageView.image = [UIImage imageNamed:@"conditions.png"];
                    break;
                case LOG_PAGE:
                    cell.textLabel.text = @"Log Messages";
                    cell.imageView.image = [UIImage imageNamed:@"log.png"];
                    break;
                case NAV_PAGE:
                    cell.textLabel.text = @"Environment";
                    cell.imageView.image = [UIImage imageNamed:@"navdata.png"];
                    break;
                case STATS_PAGE:
                    cell.textLabel.text = @"Statistics";
                    cell.imageView.image = [UIImage imageNamed:@"statistics.png"];
                    break;
                case MAP_PAGE:
                    cell.textLabel.text = @"Map";
                    cell.imageView.image = [UIImage imageNamed:@"map.png"];
                    break;
                case WP_PAGE:
                    cell.textLabel.text = @"Markers";
                    cell.imageView.image = [UIImage imageNamed:@"map-marker.png"];
                    break;
                case RC_PAGE:
                    cell.textLabel.text = @"Direct Control";
                    cell.imageView.image = [UIImage imageNamed:@"rc.png"];
                    break;
//                case DATA_PAGE:
//                    cell.textLabel.text = @"Data";
//                    cell.imageView.image = [UIImage imageNamed:@"info.png"];
//                    break;
//                case OPTIONS_PAGE:
//                    cell.textLabel.text = @"Options";
//                    cell.imageView.image = [UIImage imageNamed:@"option.png"];
//                    break;
//                case SETTINGS_PAGE:
//                    cell.textLabel.text = @"Settings";
//                    cell.imageView.image = [UIImage imageNamed:@"setting.png"];
//                    break;
                case COMMS_PAGE:
                    cell.textLabel.text = @"Comms";
                    cell.imageView.image = [UIImage imageNamed:@"radio_tower.png"];
                    break;
                case BEHAVIOR_PAGE:
                    cell.textLabel.text = @"Behaviors";
                    cell.imageView.image = [UIImage imageNamed:@"control.png"];
                    break;
                default:
                    cell = nil;
                    break;
            }
        }
            
            break;
        case 3:     //Map Actions etc.
            cell = [tableView dequeueReusableCellWithIdentifier:@"ExtraBtnCell"];
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ExtraBtnCell"];
            }
            if (currentPage == LOG_PAGE)
            {
                cell.textLabel.text = @"Clear Log";
                cell.imageView.image = [UIImage imageNamed:@"log.png"];
            }
            else if (currentPage == SYSTEM_PAGE)
            {
                cell.textLabel.text = @"Fetch Config";
                cell.imageView.image = [UIImage imageNamed:@"settings.png"];
            }
            else //if (currentPage == MAP_PAGE)
            {
                cell.textLabel.text =
                    [NSString stringWithFormat:@"%s", mapActionNames[indexPath.row]];
                cell.imageView.image = [UIImage imageNamed:@"define_location.png"];
            }
            break;
        default:
            break;
    }
    return cell;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    return NO;
}

-(void) didConnect
{
    [_viewControllers.allValues makeObjectsPerformSelector:@selector(didConnect)];
    [(UITableView*)self.view reloadData];
}

-(void) didDisconnect
{
    [(UITableView*)self.view reloadData];
    [_viewControllers.allValues makeObjectsPerformSelector:@selector(didDisconnect)];
}
-(void) didReceiveMessage: (PubSubMsg*) message
{
    int source = message.msg.header.source;
    
    if ((source == APP_XBEE) || (source == APP_OVM) || (source == ROBO_APP) || (source >= SUBSYSTEM_COUNT)) return; //mine or bad
    
    if (self.collectionController == nil)
    {
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
            AppDelegate *delegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
            self.collectionController = delegate.collectionController;
        }
        else
        {
            self.collectionController = self;
        }
    }
    
    //do we have this one?
    if (source > 0)
    {
        SubsystemViewController *ss = [_sourceCodes objectForKey:[NSNumber numberWithInt: source]];
        if (!ss) {
            //new one
            ss = [[SubsystemViewController alloc] initWithMessage: message];
            
            if (ss)
            {
                [_sourceCodes setObject:ss forKey:[NSNumber numberWithInt: source]];
                [_subsystems  setObject:ss forKey:ss.name];
                
                [ss addListener: self ];
                [self.viewControllers setObject:ss forKey:ss.name];
                
                [_collectionController addSubsystem: ss];
            }
        }
        
        [_viewControllers.allValues makeObjectsPerformSelector:@selector(didReceiveMessage:) withObject:message];
        
        if (message.msg.header.messageType == ALERT)
        {
            NSString *alertTitle = @"Overmind Alert!";
            NSString *alertCaption = [NSString stringWithFormat:@"%s", message.msg.namePayload.name];
            
            UIAlertController* alert = [UIAlertController alertControllerWithTitle:alertTitle
                                                                           message:alertCaption
                                                                    preferredStyle:UIAlertControllerStyleAlert];
            
            UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
                                                                  handler:^(UIAlertAction * action) {}];
            
            [alert addAction:defaultAction];
            [self presentViewController:alert animated:YES completion:nil];
        }
        
        [(UITableView*) self.view reloadData];
    }
}

- (void) changeSubsystemName: (SubsystemViewController*) ss oldName: (NSString*) n
{
    [self.subsystems removeObjectForKey:n];
    [self.subsystems  setObject:ss forKey:ss.name];
    [self.viewControllers removeObjectForKey:n];
    [self.viewControllers setObject:ss forKey:ss.name];
    
    [(UITableView*)self.view reloadData];
}

- (void)applicationDidEnterBackground
{
    [self didDisconnect];
}

- (void)applicationDidBecomeActive
{
    [self didConnect];
}

- (void) addSubsystem: (SubsystemStatus*) sub
{
    
}
- (UIImage*) getStatusImage: (NSString*) subsys
{
    SubsystemViewController *ss = [_subsystems objectForKey:subsys];
    
    if (ss && ss.online){
        return [UIImage imageNamed:@"online.png"];
    }
    else
    {
        return [UIImage imageNamed:@"offline.png"];
    }
}
-(void) ping:(NSTimer*) timer{
    psMessage_t msg;
    msg.header.messageType = PING_MSG;
    msg.requestPayload.requestor = ROBO_APP;
    msg.requestPayload.currentTime = time(NULL);
    [PubSubMsg sendMessage:&msg];
    gettimeofday(&_pingTime, NULL);
}

- (void) reloadTable;
{
     [(UITableView*)self.view reloadData];
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (self.collectionController == nil)
    {
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
            AppDelegate *delegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
            self.collectionController = delegate.collectionController;
        }
        else
        {
            self.collectionController = self;
        }
    }
    
    switch(indexPath.section){
        case 0:
            break;
        case 2:
        {
            NSString *ssName = [_subsystems.allKeys objectAtIndex:indexPath.row];
            if ([_collectionController presentView: ssName]) return indexPath;
        }
            break;
        case 1:
        {
            currentPage = (int) indexPath.row;
            [(UITableView*) self.view reloadData];
            switch (indexPath.row){
                case SYSTEM_PAGE:
                    if ([_collectionController presentView: @"System"]) return indexPath;
                    break;
                case POWER_PAGE:
                    if ([_collectionController presentView: @"Power"]) return indexPath;
                    break;
                case CONDITIONS_PAGE:
                    if ([_collectionController presentView: @"Conditions"]) return indexPath;
                    break;
                case LOG_PAGE:
                    if ([_collectionController presentView: @"SysLog"]) return indexPath;
                    break;
                case NAV_PAGE:
                    if ([_collectionController presentView: @"Nav"]) return indexPath;
                    break;
                case STATS_PAGE:
                    if ([_collectionController presentView: @"Stats"]) return indexPath;
                    break;
//                case DATA_PAGE:
//                    if ([_collectionController presentView: @"Data"]) return indexPath;
//                    break;
//                case OPTIONS_PAGE:
//                    if ([_collectionController presentView: @"Options"]) return indexPath;
//                    break;
//                case SETTINGS_PAGE:
//                    if ([_collectionController presentView: @"Settings"]) return indexPath;
//                    break;
                case MAP_PAGE:
                    if ([_collectionController presentView: @"Map"]) return indexPath;
                    break;
                case WP_PAGE:
                    if ([_collectionController presentView: @"Markers"]) return indexPath;
                    break;
                case RC_PAGE:
                    if ([_collectionController presentView: @"RC"]) return indexPath;
                    break;
                case COMMS_PAGE:
                    if ([_collectionController presentView: @"Comms"]) return indexPath;
                    break;
                case BEHAVIOR_PAGE:
                    if ([_collectionController presentView: @"Motion"]) return indexPath;
                    break;
                default:
                    break;
            }
            
        }
            break;
        case 3:
            if (currentPage == LOG_PAGE)
            {
                [LogViewController ClearLog];
            }
            else if (currentPage == SYSTEM_PAGE)
            {
                [_subsystems.allValues makeObjectsPerformSelector:@selector(reconfig)];
                [(UITableView*)self.view reloadData];
            }
            else //if (currentPage == MAP_PAGE)
            {
                switch (indexPath.row) {
                    case MAP_MARK:
                        [_mapController mapAction: (MapAction_enum) indexPath.row];
                        break;
                    default:
                        break;
                }
            }
            break;
        default:
            break;
    }
    return nil;
}
- (bool) presentView: (NSString*) name{
    UIViewController *controller = [_viewControllers objectForKey:name];
    controller.title = name;
    if (controller) {
        [self.navigationController pushViewController:controller animated:YES];
        return YES;
    }
    return NO;
}
- (void) statusChange: (SubsystemStatus*) ss{
    [(UITableView*)self.view reloadData];
}
@end
