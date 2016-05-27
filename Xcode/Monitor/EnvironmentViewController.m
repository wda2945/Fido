//
//  EnvironmentViewController.m
//  Monitor
//
//  Created by Martin Lane-Smith on 4/13/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "EnvironmentViewController.h"

@interface EnvironmentViewController () {
    
    psEnvironmentPayload_t  environmentReport;
    
    UITableView *tableView;
    bool connected;
}

@end

@implementation EnvironmentViewController

- (EnvironmentViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;
        
        // Custom initialization
        connected = NO;
        
        memset(&environmentReport, 0, sizeof(environmentReport));
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}
-(void) didConnect{
}
-(void) didDisconnect{
    connected = NO;
}
-(void) didReceiveMessage: (PubSubMsg*) message{
    switch(message.msg.header.messageType){
           
        case ENVIRONMENT:
            connected = YES;
            environmentReport = message.msg.environmentPayload;
            [(UITableView*)self.view reloadData];
            break;
            
        default:
            break;
    }
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)table
{
    return 1;
}
- (NSString *)tableView:(UITableView *)table titleForHeaderInSection:(NSInteger)section{
    switch (section){
        case 0:
            return @"Environment";
            break;
        default:
            return @"";
            break;
    }
}
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section
{
    switch (section){
        case 0: //Environment
            return 13;
            break;
        default:
            return 0;
            break;
    }
}

- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    switch (indexPath.section){

        case 0:     //environment
        {
            UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"NavViewCell"];
            
            if (!cell)
            {
                cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue2 reuseIdentifier:@"NavViewCell"];
            }
            
            switch (indexPath.row) {
                case 0:
                    cell.textLabel.text = @"Battery Volts:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%0.1f", environmentReport.batteryVolts / 10.0f];
                    break;
                case 1:
                    cell.textLabel.text = @"Battery Amps:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%0.1f", environmentReport.batteryAmps / 10.0f];
                    break;
                case 2:
                    cell.textLabel.text = @"Max Amps:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%0.1f", environmentReport.maxAmps / 10.0f];
                    break;
                case 3:
                    cell.textLabel.text = @"Battery Ah:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%0.1f", environmentReport.batteryAh / 10.0f];
                    break;
                case 4:
                    cell.textLabel.text = @"Solar Volts:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%0.1f", environmentReport.solarVolts / 10.0f];
                    break;
                case 5:
                    cell.textLabel.text = @"Solar Amps:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%0.1f", environmentReport.solarAmps / 10.0f];
                    break;
                case 6:
                    cell.textLabel.text = @"Charge Volts:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%0.1f", environmentReport.chargeVolts / 10.0f];
                    break;
                case 7:
                    cell.textLabel.text = @"Charge Amps:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%0.1f", environmentReport.chargeAmps / 10.0f];
                    break;
                case 8:
                    cell.textLabel.text = @"Internal Temp:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%i", environmentReport.internalTemp];
                    break;
                case 9:
                    cell.textLabel.text = @"External Temp:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%i", environmentReport.externalTemp];
                    break;
                case 10:
                    cell.textLabel.text = @"Humidity:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%i", environmentReport.relativeHumidity];
                    break;
                case 11:
                    cell.textLabel.text = @"Ambient Light:";
                    cell.detailTextLabel.text = [NSString stringWithFormat:@"%i", environmentReport.ambientLight];
                    break;
                case 12:
                    cell.textLabel.text = @"Raining?:";
                    if (environmentReport.isRaining)
                    {
                        cell.detailTextLabel.text = @"Yes";
                    }
                    else
                    {
                        cell.detailTextLabel.text = @"No";
                    }
                    break;
                default:
                    break;
            }
            return cell;
        }
            break;
   }
    
    return nil;
}
- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
