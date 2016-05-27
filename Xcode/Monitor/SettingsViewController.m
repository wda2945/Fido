//
//  SettingsViewController.m
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 2/2/15.
//  Copyright (c) 2015 Martin Lane-Smith. All rights reserved.
//

#import "SettingsViewController.h"
#import "MasterViewController.h"
#import "SubsystemViewController.h"
#import "SettingsDetailViewController.h"

@interface SettingsViewController ()
{
    UITableView *tableView;
    SettingsDetailViewController *detailController;
}
@end

@implementation SettingsViewController

- (SettingsViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;
    }
    
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        detailController = [[SettingsDetailViewController alloc] initWithNibName:@"SettingsDetail-iPad" bundle:nil];
    }
    else{
        detailController = [[SettingsDetailViewController alloc] initWithNibName:@"SettingsDetail-iPhone" bundle:nil];
    }
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
    return ss.settings.count;
    return 0;
}

- (UITableViewCell *)tableView:(UITableView *)_tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath{
    SubsystemViewController *ss =
    [[MasterViewController getMasterViewController].subsystems objectForKey:
     [[MasterViewController getMasterViewController].subsystems.allKeys objectAtIndex:indexPath.section]];
    
    UITableViewCell *cell = [_tableView dequeueReusableCellWithIdentifier:@"SettingsCell"];
    
    if (!cell)
    {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"SettingsCell"];
    }
    if (ss.settings.count > indexPath.row)
    {
        NSString *settingName = [ss.sortedSettingNames objectAtIndex:indexPath.row];
        NSMutableDictionary *dict = [ss.settings objectForKey:settingName];
        cell.textLabel.text = settingName;
        cell.detailTextLabel.text = [(NSNumber*)[dict objectForKey:@"value"] stringValue];
    }
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    cell.imageView.image = [UIImage imageNamed:@"setting.png"];
    return cell;
}
- (NSIndexPath *)tableView:(UITableView *)_tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    SubsystemViewController *ss =
    [[MasterViewController getMasterViewController].subsystems objectForKey:
     [[MasterViewController getMasterViewController].subsystems.allKeys objectAtIndex:indexPath.section]];

    NSString *settingName = [ss.sortedSettingNames objectAtIndex:indexPath.row];
    NSMutableDictionary *dict = [ss.settings objectForKey:settingName];
    
    [detailController settingsDict:dict];
    
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        
        detailController.modalPresentationStyle = UIModalPresentationPopover;

        // Get the popover presentation controller and configure it.
        UIPopoverPresentationController *presentationController = [detailController popoverPresentationController];
        presentationController.permittedArrowDirections = UIPopoverArrowDirectionAny;
        presentationController.sourceView = self.view;
        presentationController.sourceRect = [_tableView rectForRowAtIndexPath:indexPath];
        
        [self presentViewController:detailController animated: YES completion: nil];
    }
    else
    {
        [self.navigationController pushViewController:detailController animated:YES];
    }
    return indexPath;
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
        case SETTING:
        case NEW_SETTING:
        case OPTION:
        case SET_OPTION:
        case CONFIG_DONE:
            [tableView reloadData];
            break;
        default:
            break;
    }
}


@end
