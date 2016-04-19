//
//  StatusViewController.m
//  Monitor
//
//  Created by Martin Lane-Smith on 4/14/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "SubsystemViewController.h"
#import "MasterViewController.h"
#import "BehaviorViewController.h"
#import "SystemViewController.h"
#import "SettingsDetailViewController.h"
#import "PubSubData.h"
#import "LogViewController.h"
#import "AppDelegate.h"
#include <sys/time.h>

@interface SubsystemViewController ()
{
    SettingsDetailViewController *detailController;
    bool currentlyOnline;
    long latency;
}
@property (strong) NSMutableDictionary *switches;
@property (strong) NSMutableArray      *switchList;
@property (strong) NSMutableArray *listeners;
@property (strong) NSTimer *onlineTimeout;
@property (strong) NSTimer *configTimer;

- (void) sendForConfig;
@end

@implementation SubsystemViewController

- (void) ssInitFailed
{
    if (!_initErrors)
    {
        //only report once
        NSString *alertTitle = [NSString stringWithFormat:@"%@ INIT", self.name];
        NSString *alertCaption = [NSString stringWithFormat:@"%@ reported initialization failure!", self.name];
        
        UIAlertController* alert = [UIAlertController alertControllerWithTitle:alertTitle
                                                                       message:alertCaption
                                                                preferredStyle:UIAlertControllerStyleAlert];
        
        UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
                                                              handler:^(UIAlertAction * action) {}];
        
        [alert addAction:defaultAction];
        [[MasterViewController getMasterViewController] presentViewController:alert animated:YES completion:nil];
        
        _initErrors = true;
    }
}

- (SubsystemViewController*) initWithMessage: (PubSubMsg*) message
{
    self = [super init];
    if (self){
        self.listeners = [NSMutableArray array];
        
        self.sourceCode = message.msg.header.source;
        
        self.name = [NSString stringWithFormat:@"%s", subsystemNames[self.sourceCode]];
        
        if ((message.msg.header.messageType == SS_ONLINE) && (message.msg.responsePayload.flags & RESPONSE_INIT_ERRORS))
        {
            [self ssInitFailed];
        }

        [LogViewController logAppMessage:[NSString stringWithFormat:@"Subsystem %@ added", self.name]];

        self.stats      = [NSMutableDictionary dictionary];
        self.configured = NO;
        self.initErrors = false;

        [self sendForConfig];
        latency = -1;
    }
    return self;
    
}

- (void) notify {
    [_listeners makeObjectsPerformSelector:@selector(statusChange:) withObject:self];
    [(UITableView*)self.view reloadData];
}

- (void) ssOffline: (NSTimer*) timer{
    self.online = NO;
    [self notify];
    [LogViewController logAppMessage:[NSString stringWithFormat:@"%@ timed out", self.name]];

}

- (void) reconfig
{
    if (self.configTimer)
    {
        [self.configTimer invalidate];
        self.configTimer    = nil;
    }
    self.configured     = NO;
    [self sendForConfig];
    
    if (self.sourceCode == OVERMIND)
    {
        [(BehaviorViewController*)[MasterViewController getMasterViewController].behaviorViewController clearScripts];
    }
}

- (void) restartOnlineTimeout {
    if (self.onlineTimeout) {
        [self.onlineTimeout invalidate];
        self.onlineTimeout = nil;
    }
    
    [self sendForConfig];

    if (!self.online)
    {
        self.online = YES;
        [LogViewController logAppMessage:[NSString stringWithFormat:@"%@ online", self.name]];
        [self notify];
    }
    
    self.onlineTimeout =     [NSTimer scheduledTimerWithTimeInterval:TIMEOUT_DURATION
                                                              target:self
                                                            selector:@selector(ssOffline:)
                                                            userInfo:nil
                                                             repeats:NO];
}

-(void) didConnect
{
    [self sendForConfig];
}
-(void) didDisconnect
{
    self.online     = NO;
    [self notify];
    if (self.onlineTimeout) {
        [self.onlineTimeout invalidate];
        self.onlineTimeout = nil;
    }
    if (self.configTimer) {
        [self.configTimer invalidate];
        self.configTimer = nil;
    }
}

-(void) didReceiveMessage: (PubSubMsg*) message
{
    if (self.sourceCode == message.msg.header.source){
        
        [self restartOnlineTimeout];

        switch (message.msg.header.messageType){
            case PING_RESPONSE:
            {
                char ssName[4];
                
                if (message.msg.responsePayload.requestor == ROBO_APP)
                {
                    //response to my ping, check latency
                    struct timeval pingTime, responseTime, interval;
                    
                    pingTime = [MasterViewController getMasterViewController].pingTime;
                    gettimeofday(&responseTime, NULL);
                    
                    if (!timeval_subtract(&interval, &responseTime, &pingTime))
                    {
                        latency = interval.tv_sec * 1000 + interval.tv_usec / 1000;
                    }
                    else{
                        latency = -1;
                    }
                }
                
                //fix SS name if needed
                strncpy(ssName, message.msg.responsePayload.subsystem,  3);
                ssName[3] = '\0';
                
                if (![self.name isEqualToString:[NSString stringWithFormat:@"%s", ssName]])
                {
                    NSString *oldName = [self.name copy];
                    self.name       = [NSString stringWithFormat:@"%s", ssName];

                    [[MasterViewController getMasterViewController]  changeSubsystemName: self oldName: oldName];
                    
                    [self notify];
                }
                
                if (message.msg.responsePayload.flags & RESPONSE_INIT_ERRORS)
                {
                    NSLog(@"InitFailed");
                }
                
                [(UITableView*)self.view reloadData];

            }
                break;
            case SS_ONLINE:
            {
                char ssName[4];
                strncpy(ssName, message.msg.responsePayload.subsystem,  3);
                ssName[3] = '\0';
                
                if (![self.name isEqualToString:[NSString stringWithFormat:@"%s", ssName]])
                {
                    NSString *oldName = [self.name copy];
                    self.name       = [NSString stringWithFormat:@"%s", ssName];
                    [self notify];
                    [(UITableView*)self.view reloadData];
                    [[MasterViewController getMasterViewController]  changeSubsystemName: self oldName: oldName];
                }
                if (message.msg.responsePayload.flags & RESPONSE_INIT_ERRORS)
                {
                    [self ssInitFailed];
                }

            }
                break;
            case SETTING:
                if (message.msg.settingPayload.subsystem == ROBO_APP)
                {
                    NSString *key = [NSString stringWithFormat:@"%s",message.msg.settingPayload.name];
                    
                    NSMutableDictionary *settingDict = [self.settings objectForKey:key];
                    if (!settingDict){
                        settingDict = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                       key, @"name",
                                       [NSNumber numberWithFloat:message.msg.settingPayload.min], @"min",
                                       [NSNumber numberWithFloat:message.msg.settingPayload.max], @"max",
                                       [NSNumber numberWithFloat:message.msg.settingPayload.value], @"value",
                                       [NSNumber numberWithBool:NO], @"integer",
                                       nil];
                        [self.settings setObject:settingDict forKey:key];
                        self.sortedSettingNames =
                            [_settings.allKeys sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
                    }
                    else
                    {
                        //just set new value
                        [settingDict setObject:[NSNumber numberWithFloat:message.msg.settingPayload.value] forKey:@"value"];
                    }
                    [(UITableView*)self.view reloadData];
                    
                }
                break;
            case OPTION:
                if (message.msg.optionPayload.subsystem == ROBO_APP)
                {
                    NSString *key = [NSString stringWithFormat:@"%s",message.msg.optionPayload.name];
                    
                    if (message.msg.optionPayload.max == 1)
                    {
                        //on-off option
                        NSMutableDictionary *optionDict = [self.options objectForKey:key];
                        if (!optionDict){
                            optionDict = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                          key, @"name",
                                          [NSNumber numberWithInt: message.msg.optionPayload.min], @"min",
                                          [NSNumber numberWithInt: message.msg.optionPayload.max], @"max",
                                          [NSNumber numberWithInt: message.msg.optionPayload.value], @"value",
                                          nil];
                            [self.options setObject:optionDict forKey:key];
                            self.sortedOptionNames =
                            [_options.allKeys sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
                        }
                        else
                        {
                            //just set new value
                            [optionDict setObject:[NSNumber numberWithInt:message.msg.optionPayload.value] forKey:@"value"];
                        }
                    }
                    else
                    {
                        //integer setting
                        NSMutableDictionary *optionDict = [self.settings objectForKey:key];
                        if (!optionDict){
                            optionDict = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                          key, @"name",
                                          [NSNumber numberWithInt: message.msg.optionPayload.min], @"min",
                                          [NSNumber numberWithInt: message.msg.optionPayload.max], @"max",
                                          [NSNumber numberWithInt: message.msg.optionPayload.value], @"value",
                                          [NSNumber numberWithBool:YES], @"integer",
                                          nil];
                            [self.settings setObject:optionDict forKey:key];
                            self.sortedSettingNames =
                            [_settings.allKeys sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
                       }
                        else
                        {
                            //just set new value
                            [optionDict setObject:[NSNumber numberWithInt:message.msg.optionPayload.value] forKey:@"value"];
                        }
                    }
                    [(UITableView*)self.view reloadData];
                }
                break;
                
            case CONFIG_DONE:
                //check all the expected messages have arrived
                if (message.msg.configPayload.requestor == ROBO_APP)
                {
                    if (!self.configured && (message.msg.configPayload.count
                                             == (_settings.count + _options.count + (self.sourceCode != OVERMIND ? 0 :
                                                                                     [MasterViewController getMasterViewController].behaviorViewController.availableScripts.count))))
                    {
                        [LogViewController logAppMessage:[NSString stringWithFormat:@"%@ configured", self.name]];
                        self.configured = YES;
                        [self notify];
                    }
                    else{
                        NSLog(@"Config incomplete (%@)", _name);
//                        [self sendForConfig];
                    }
                }
                break;
                
            case FLOAT_DATA:
            {
                NSString *key = [NSString stringWithFormat:@"%s",message.msg.nameFloatPayload.name];
                
                NSMutableDictionary *infoDict = [self.info objectForKey:key];
                if (!infoDict){
                    infoDict = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                key, @"name",
                                [NSNumber numberWithFloat: message.msg.nameFloatPayload.value], @"value",
                                nil];
                    [self.info setObject:infoDict forKey:key];
                    self.sortedInfoNames =
                    [_info.allKeys sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
                }
                else
                {
                    //just set new value
                    [infoDict setObject:[NSNumber numberWithFloat:message.msg.nameFloatPayload.value] forKey:@"value"];
                }
            }
                break;
            case INT_DATA:
            {
                NSString *key = [NSString stringWithFormat:@"%s",message.msg.nameFloatPayload.name];
                
                NSMutableDictionary *infoDict = [self.info objectForKey:key];
                if (!infoDict){
                    infoDict = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                key, @"name",
                                [NSNumber numberWithInt: message.msg.nameIntPayload.value], @"value",
                                nil];
                    [self.info setObject:infoDict forKey:key];
                    self.sortedInfoNames =
                    [_info.allKeys sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
                }
                else
                {
                    //just set new value
                    [infoDict setObject:[NSNumber numberWithInt:message.msg.nameIntPayload.value] forKey:@"value"];
                }
            }
            default:
                break;
        }
    }
}

- (void) sendForConfig
{
    if (!self.configTimer && !self.configured)
    {
        self.info               = [NSMutableDictionary dictionary];
        self.sortedInfoNames    = [NSArray array];
        self.settings           = [NSMutableDictionary dictionary];
        self.sortedSettingNames = [NSArray array];
        self.options            = [NSMutableDictionary dictionary];
        self.sortedOptionNames  = [NSArray array];
        self.switches           = [NSMutableDictionary dictionary];
        self.switchList         = [NSMutableArray   array];
        
        psMessage_t message;
        message.header.messageType = CONFIG;
        message.configPayload.responder = self.sourceCode;
        message.configPayload.requestor = ROBO_APP;
        [PubSubMsg sendMessage: &message];
        
        self.configTimer = [NSTimer scheduledTimerWithTimeInterval: CONFIG_TIMEOUT
                                                            target:self
                                                          selector:@selector(configTimerFired:)
                                                          userInfo:nil
                                                           repeats:YES];
        
        [LogViewController logAppMessage:[NSString stringWithFormat:@"%@ sent for config", self.name]];
        [(UITableView*)self.view reloadData];
    }
}

- (void) configTimerFired: (NSTimer*) timer
{
    NSLog(@"Config timer fired (%@)", _name);
    [self.configTimer invalidate];
    self.configTimer = nil;
    [self sendForConfig];
}

- (void) addListener: (id <StatusUpdate>) _listener
{
    [_listeners addObject:_listener];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        detailController = [[SettingsDetailViewController alloc] initWithNibName:@"SettingsDetail-iPad" bundle:nil];
    }
    else{
        detailController = [[SettingsDetailViewController alloc] initWithNibName:@"SettingsDetail-iPhone" bundle:nil];
    }
}

- (void) viewWillAppear:(BOOL)animated  {
    self.switches    = [NSMutableDictionary dictionary];
    self.switchList  = [NSMutableArray   array];
    
    [super viewWillAppear:animated];
}

//Table View
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView{
    return 4;
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section{
    switch(section){
        case 0:
            return _name;
            break;
        case 1:
            return @"Information";
            break;
        case 2:
            return @"Options";
            break;
        case 3:
            return @"Settings";
            break;
        default:
            return nil;
            break;
    }
}
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section
{
    switch(section){
        case 0:
            return 2;
            break;
        case 1:
            return _info.count;
            break;
        case 2:
            return _options.count;
            break;
        case 3:
            return _settings.count;
            break;
        default:
            return 0;
            break;
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath{
    UITableViewCell *cell;
    
    switch(indexPath.section){
        case 0:
        {
            cell = [tableView dequeueReusableCellWithIdentifier:@"StatusCell"];
            
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"StatusCell"];
            }
            switch (indexPath.row)
            {
                case 0:
                    if (_online){
                        cell.textLabel.text = @"Online";
                        cell.imageView.image = [UIImage imageNamed:@"online.png"];
                    } else{
                        cell.textLabel.text = @"Offline";
                        cell.imageView.image = [UIImage imageNamed:@"offline.png"];
                    }
                    break;
                case 1:
                    if (latency >=0)
                    {
                        cell.textLabel.text = [NSString stringWithFormat:@"Latency %limS", latency];
                    }
                    else{
                        cell.textLabel.text = @"Latency unknown";
                    }
                    cell.imageView.image = [UIImage imageNamed:@"setting.png"];
                    break;
            }
        }
            break;
        case 1:
        {
            cell = [tableView dequeueReusableCellWithIdentifier:@"InfoCell"];
            
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"InfoCell"];
            }
            
            if (_info.count > indexPath.row)
            {
                NSString *infoName = [_sortedInfoNames objectAtIndex: indexPath.row];
                NSMutableDictionary *dict = [_info objectForKey:infoName];
                cell.textLabel.text = infoName;
                cell.detailTextLabel.text =  [NSString stringWithFormat:@"%0.2f", [(NSNumber*)[dict objectForKey:@"value"] floatValue]];
            }
            cell.imageView.image = [UIImage imageNamed:@"info.png"];
        }
            break;
        case 2:
        {
            cell = [tableView dequeueReusableCellWithIdentifier:@"OptionCell"];
            
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"OptionCell"];
            }
            if (_options.count > indexPath.row)
            {
                NSString *optionName = [_sortedOptionNames objectAtIndex: indexPath.row];
                NSMutableDictionary *dict = [_options objectForKey:optionName];
                
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
            
        }
            break;
        case 3:
        {
            cell = [tableView dequeueReusableCellWithIdentifier:@"SettingsCell"];
            
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"SettingsCell"];
            }
            if (_settings.count > indexPath.row)
            {
                NSString *settingName = [_sortedSettingNames objectAtIndex:indexPath.row];
                NSMutableDictionary *dict = [_settings objectForKey:settingName];
                cell.textLabel.text = settingName;
                cell.detailTextLabel.text = [(NSNumber*)[dict objectForKey:@"value"] stringValue];
            }
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            cell.imageView.image = [UIImage imageNamed:@"setting.png"];
            
        }
            break;
        default:
            break;
    }
    return cell;
    
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath{
    switch(indexPath.section){
        case 3:
        {
            NSString *settingName = [_sortedSettingNames objectAtIndex:indexPath.row];
            NSMutableDictionary *dict = [_settings objectForKey:settingName];
            
            [detailController settingsDict:dict];
            
            if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
                detailController.modalPresentationStyle = UIModalPresentationPopover;
                
                [self presentViewController:detailController animated: YES completion: nil];
                
                // Get the popover presentation controller and configure it.
                UIPopoverPresentationController *presentationController =
                [detailController popoverPresentationController];
                presentationController.permittedArrowDirections = UIPopoverArrowDirectionAny;
                presentationController.sourceView = self.view;
                presentationController.sourceRect = [tableView rectForRowAtIndexPath:indexPath];

            }
            else
            {
                [self.navigationController pushViewController:detailController animated:YES];
            }
            return indexPath;
        }
            break;
        default:
            return nil;
            break;
    }
}

- (void) swChanged: (UISwitch*) sw{
    psMessage_t msg;
    
    NSString *optionName = [_switchList objectAtIndex:sw.tag];
    [optionName getCString:msg.optionPayload.name maxLength:PS_NAME_LENGTH encoding:NSASCIIStringEncoding];
    
    msg.header.messageType = SET_OPTION;
    msg.optionPayload.value = (uint8_t) sw.on;
    
    [PubSubMsg sendMessage:&msg];
    
    NSMutableDictionary *dict = [_options objectForKey:optionName];
    [dict setObject: [NSNumber numberWithBool:sw.on] forKey:@"value"];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

/* Subtract the ‘struct timeval’ values X and Y,
 storing the result in RESULT.
 Return 1 if the difference is negative, otherwise 0. */
int timeval_subtract (result, x, y)
struct timeval *result, *x, *y;
{
    /* Perform the carry for the later subtraction by updating y. */ if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }
    /* Compute the time remaining to wait. tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

@end
