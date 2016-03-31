//
//  StatsViewController.m
//  Monitor
//
//  Created by Martin Lane-Smith on 4/13/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "StatsViewController.h"
#import "MasterViewController.h"
#import "PubSubMsg.h"
#import "PubSubData.h"

@interface StatsViewController (){
    NSMutableArray      *stats;
    UITableView         *tableView;
}
@end

@implementation StatsViewController


- (StatsViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}
- (void)viewWillAppear:(BOOL)animated {
    
    [PubSubMsg sendSimpleMessage:GEN_STATS];

    [super viewWillAppear:animated];
    [(UITableView*)self.view reloadData];
}
- (void) viewDidDisappear:(BOOL)animated{
    for (SubsystemViewController *ss in [MasterViewController getMasterViewController].subsystems.allValues){
        [ss.stats removeAllObjects];
    }
    [super viewDidDisappear:animated];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)table{
    return [MasterViewController getMasterViewController].subsystems.count;
}
- (NSString *)tableView:(UITableView *)table titleForHeaderInSection:(NSInteger)section{

        return [[MasterViewController getMasterViewController].subsystems.allKeys objectAtIndex:section];
    
}
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section
{

        SubsystemViewController *ss =
                        [[MasterViewController getMasterViewController].subsystems objectForKey:
                        [[MasterViewController getMasterViewController].subsystems.allKeys objectAtIndex:section]];
        return ss.stats.count;
}

- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)indexPath{
    NSMutableDictionary *dict;

        SubsystemViewController *ss =
            [[MasterViewController getMasterViewController].subsystems objectForKey:
                [[MasterViewController getMasterViewController].subsystems.allKeys objectAtIndex:indexPath.section]];
    
        dict = [ss.stats.allValues objectAtIndex:indexPath.row];

    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"DashboardCell"];
    
    if (!cell)
    {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"DashboardCell"];
    }
    cell.textLabel.text = [dict objectForKey:@"name"];
    cell.detailTextLabel.text = [dict objectForKey:@"value"];
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
    
    int ssNumber = message.msg.header.source;
    SubsystemViewController *ss =
    [[MasterViewController getMasterViewController].sourceCodes objectForKey:[NSNumber numberWithInt: ssNumber]];
    
    if (!ss) return;
    
    switch(message.msg.header.messageType){
        case TASK_STATS:
        {
            NSString *taskName = [NSString stringWithFormat:@"%s",message.msg.taskStatsPayload.taskName];
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    taskName, @"name",
                    [NSString stringWithFormat:@"%-2i%%, %i bytes",
                     message.msg.taskStatsPayload.percentCPU,
                     message.msg.taskStatsPayload.stackHeadroom * 2], @"value",
                    nil];
            [ss.stats setObject:dict forKey:taskName];
        }
            break;
        case SYS_STATS:
        {
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    @"Total CPU", @"name",
                    [NSString stringWithFormat:@"%-3i%%",message.msg.sysStatsPayload.cpuPercentage], @"value",
                    nil];
            [ss.stats setObject:dict forKey:@"cpu"];
            dict = [NSDictionary dictionaryWithObjectsAndKeys:
                    @"Free Memory", @"name",
                    [NSString stringWithFormat:@"%li bytes",message.msg.sysStatsPayload.freeHeap], @"value",
                    nil];
            [ss.stats setObject:dict forKey:@"memory"];
        }
            break;
       default:
            break;
    }
    [(UITableView*)self.view reloadData];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
