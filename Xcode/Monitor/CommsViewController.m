//
//  CommsViewController.m
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 2/13/15.
//  Copyright (c) 2015 Martin Lane-Smith. All rights reserved.
//

#import "CommsViewController.h"
#import "MasterViewController.h"
#import "ConditionsViewController.h"
#import "PubSubMsg.h"
#import "PubSubData.h"
#import "AppDelegate.h"


@interface CommsViewController () {
    UITableView *tableView;
    
    NSMutableDictionary *commStats;
    NSTimer             *minuteTimer;
    
    NSString *connectionCaptions[SERVER_COUNT];
    bool connected[SERVER_COUNT];
    float   connectionLatency[SERVER_COUNT];
    
    UIImage *imgOnline;
    UIImage *imgConnected;
    UIImage *imgOffline;
    UIImage *imgDisabled;

    UISwitch *sw[SERVER_COUNT+1];
    
    NSString *serverNames[SERVER_COUNT];
}
- (void) bleStats: (NSTimer *) obj;
- (void) swChanged: (UISwitch*) sw;

@end

@implementation CommsViewController

- (CommsViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;
        commStats = [NSMutableDictionary dictionary];
        
        minuteTimer = [NSTimer scheduledTimerWithTimeInterval: 60
                                                       target: self
                                                     selector: @selector(bleStats:)
                                                     userInfo: nil
                                                      repeats: YES];
        for (int i=0; i<SERVER_COUNT; i++)
        {
            connectionCaptions[i] = @"";
            connectionLatency[i] = -1;
            connected[i] = NO;
            sw[i] = [[UISwitch alloc] init];
            sw[i].tag = i;
            [sw[i] addTarget:self action:@selector(swChanged:) forControlEvents:UIControlEventValueChanged];
            
            if (i == FIDO_XBEE_SERVER) sw[i].on = YES;
            else sw[i].on = NO;
            
            [self swChanged: sw[i]];
        }
        serverNames[FIDO_OVM_SERVER]    = @"OVM Agent";
        serverNames[FIDO_XBEE_SERVER]   = @"XBee Agent";
        serverNames[BLE_SERVER]         = @"BLE";
        
        imgOnline = [UIImage imageNamed:@"online.png"];
        imgOffline = [UIImage imageNamed:@"offline.png"];
        imgConnected = [UIImage imageNamed:@"connected.png"];
        imgDisabled = [UIImage imageNamed:@"disabled.png"];
    }
    return self;
}
- (void) newConnectionCaption: (NSString*) caption forServer: (int) index connected: (bool) conn
{
    connectionCaptions[index] = [caption copy];
    connected[index] = conn;
    [tableView reloadData];
}
- (void) newLatencyMeasure: (float) millisecs forServer: (int) index
{
    connectionLatency[index] = millisecs;
    [tableView reloadData];
}

- (UIImage*) getConnectionIcon: (int) index
{
    if (!sw[index].on)
    {
        return imgDisabled;
    }
    
    if (index == FIDO_XBEE_SERVER)
    {
        if (!connected[FIDO_XBEE_SERVER])
        {
            return imgOffline;
        }
        else{
            if ([[MasterViewController getMasterViewController].conditionsViewController isConditionSet: XBEE_MCP_ONLINE])
            {
                return imgOnline;
            }
            else{
                return imgConnected;
            }
        }
    }
    else{
        return (connected[index] ? imgOnline : imgOffline);
    }
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)table{
    return commStats.allKeys.count + SERVER_COUNT;
}
- (NSString *)tableView:(UITableView *)table titleForHeaderInSection:(NSInteger)section{
    switch(section)
    {
        case FIDO_OVM_SERVER:
            return @"Fido via UDP";
            break;
        case FIDO_XBEE_SERVER:
            return @"Fido via XBee Proxy";
            break;
        case BLE_SERVER:
            return @"Fido via BLE";
            break;
        default:
            return [commStats.allKeys objectAtIndex:section - SERVER_COUNT];
            break;
    }
}

- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section
{
    switch(section)
    {
        case FIDO_OVM_SERVER:
        case FIDO_XBEE_SERVER:
        case BLE_SERVER:
            return 2;
            break;
        default:
        {
            NSArray *commStatsArray = [commStats objectForKey:[commStats.allKeys objectAtIndex:section - SERVER_COUNT ]];
            return commStatsArray.count;
        }
            break;
    }
}

- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)indexPath{
    NSMutableDictionary *dict;
    UITableViewCell *cell;
    
    switch(indexPath.section)
    {
        case FIDO_OVM_SERVER:
        case FIDO_XBEE_SERVER:
        case BLE_SERVER:
        {
            cell = [tableView dequeueReusableCellWithIdentifier:@"CommsStatsCell1"];
            
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"CommsStatsCell1"];
            }
            switch(indexPath.row)
            {
                case 0:
                    cell.textLabel.text = connectionCaptions[indexPath.section];
                    cell.accessoryView  = sw[indexPath.section];
                    
                    cell.imageView.image =  [self getConnectionIcon: (int) indexPath.section];
                    
                    break;
                default:
                    if (connectionLatency[indexPath.section] > 0)
                    {
                        cell.textLabel.text = [NSString stringWithFormat:@"Latency %i mS", (int)connectionLatency[indexPath.section]];
                    }
                    else{
                        cell.textLabel.text = @"Latency unknown";
                    }
                    cell.accessoryView = nil;
                    break;
            }
        }
            break;
        default:
        {
            cell = [tableView dequeueReusableCellWithIdentifier:@"CommsStatsCell2"];
            
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"CommsStatsCell2"];
            }
            NSArray *commStatsArray = [commStats objectForKey:[commStats.allKeys objectAtIndex:indexPath.section  - SERVER_COUNT]];
            dict = [commStatsArray objectAtIndex:indexPath.row];
            
            cell.textLabel.text = [dict objectForKey:@"name"];
            cell.detailTextLabel.text = [dict objectForKey:@"value"];
        }
            break;
    }
    return cell;
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    return nil;
}

-(void) didConnect{
    
}
-(void) didDisconnect{
    
}
-(void) didReceiveMessage: (PubSubMsg*) message{
    
    NSDictionary *dict = nil;
    
    switch(message.msg.header.messageType){
        case COMMS_STATS:
        {
            NSString        *source = [NSString stringWithFormat:@"%s", subsystemNames[message.msg.header.source]];
            NSString        *link   = [NSString stringWithFormat:@"%s", message.msg.commsStatsPayload.destination];
            NSString        *key    = [NSString stringWithFormat:@"%@ >> %@ Comms", source, link];
            [commStats  removeObjectForKey:key];
            NSMutableArray  *statList = [NSMutableArray array];

            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSString stringWithFormat:@"%@ -> %@", source, link], @"name",
                    [NSString stringWithFormat:@"%i",message.msg.commsStatsPayload.messagesSent], @"value",
                    nil];
            [statList addObject:dict];
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSString stringWithFormat:@"%@ ->| %@ address", source, link], @"name",
                    [NSString stringWithFormat:@"%i",message.msg.commsStatsPayload.addressDiscarded], @"value",
                    nil];
            [statList addObject:dict];
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSString stringWithFormat:@"%@ ->| %@ congestion", source, link], @"name",
                    [NSString stringWithFormat:@"%i",message.msg.commsStatsPayload.congestionDiscarded], @"value",
                    nil];
            [statList addObject:dict];
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSString stringWithFormat:@"%@ ->| %@ syslog", source, link], @"name",
                    [NSString stringWithFormat:@"%i",message.msg.commsStatsPayload.logMessagesDiscarded], @"value",
                    nil];
            [statList addObject:dict];
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSString stringWithFormat:@"%@ ->| %@ send errors", source, link], @"name",
                    [NSString stringWithFormat:@"%i",message.msg.commsStatsPayload.sendErrors], @"value",
                    nil];
            [statList addObject:dict];
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSString stringWithFormat:@"%@ <- %@", source, link], @"name",
                    [NSString stringWithFormat:@"%i",message.msg.commsStatsPayload.messagesReceived], @"value",
                    nil];
            [statList addObject:dict];
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSString stringWithFormat:@"%@ |<- %@ ignored", source, link], @"name",
                    [NSString stringWithFormat:@"%i",message.msg.commsStatsPayload.addressIgnored], @"value",
                    nil];
            [statList addObject:dict];
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSString stringWithFormat:@"%@ |<- %@ parse errors", source, link], @"name",
                    [NSString stringWithFormat:@"%i",message.msg.commsStatsPayload.parseErrors], @"value",
                    nil];
            [statList addObject:dict];
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    [NSString stringWithFormat:@"%@ |<- %@ receive errors", source, link], @"name",
                    [NSString stringWithFormat:@"%i",message.msg.commsStatsPayload.receiveErrors], @"value",
                    nil];
            [statList addObject:dict];
            [commStats setObject:statList forKey: key];
        }
            break;
        default:
            break;
    }
    [(UITableView*)self.view reloadData];
}

- (void) swChanged: (UISwitch*) enablesw
{
    [(AppDelegate*)[UIApplication sharedApplication].delegate enableComms: (int) enablesw.tag enabled: enablesw.on];
}

- (void) bleStats: (NSTimer *) obj
{
    AppDelegate     *appDelegate = (AppDelegate *)[UIApplication sharedApplication].delegate;
    NSDictionary    *dict;
    for (int i = 0; i<SERVER_COUNT; i++)
    {
        NSString  *source   = @"APP";
        NSString  *link     = serverNames[i];
        NSString  *key      = [NSString stringWithFormat:@"%@ >> %@ Comms", source, link];
        [commStats  removeObjectForKey:key];
        NSMutableArray  *statList = [NSMutableArray array];
        
        dict = [NSDictionary dictionaryWithObjectsAndKeys:
                [NSString stringWithFormat:@"%@ -> %@", source, link], @"name",
                [NSString stringWithFormat:@"%li",[appDelegate MessagesSent:i]], @"value",
                nil];
        [statList addObject:dict];
        dict = [NSDictionary dictionaryWithObjectsAndKeys:
                [NSString stringWithFormat:@"%@ ->| %@ send errors", source, link], @"name",
                [NSString stringWithFormat:@"%li",[appDelegate SendErrors:i]], @"value",
                nil];
        [statList addObject:dict];
        dict = [NSDictionary dictionaryWithObjectsAndKeys:
                [NSString stringWithFormat:@"%@ <- %@", source, link], @"name",
                [NSString stringWithFormat:@"%li",[appDelegate MessagesReceived:i]], @"value",
                nil];
        [statList addObject:dict];
        dict = [NSDictionary dictionaryWithObjectsAndKeys:
                [NSString stringWithFormat:@"%@ |<- %@ receive errors", source, link], @"name",
                [NSString stringWithFormat:@"%li", [appDelegate ReceiveErrors:i]], @"value",
                nil];
        [statList addObject:dict];
        [commStats setObject:statList forKey:key];
    }
    
    if (self.view)
        [(UITableView*)self.view reloadData];
}
@end
