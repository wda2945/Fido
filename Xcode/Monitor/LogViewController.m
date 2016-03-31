//
//  DetailViewController.m
//  Monitor
//
//  Created by Martin Lane-Smith on 4/12/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "LogViewController.h"
#import "MasterViewController.h"
#import "SubsystemViewController.h"
#import "PubSubData.h"

LogViewController *thisLogViewController;

@interface LogViewController () {
    bool viewLoaded;
    UITableView *tableView;
}

@property (strong, nonatomic) NSMutableArray *logMessages;
@property (strong, nonatomic) NSDateFormatter *dateFormatter;

- (void) logMessage: (NSString*) log detail: (NSString*) detail icon: (UIImage*) icon;
- (void) appMessage: (NSString*) log ;
@end

@implementation LogViewController

- (LogViewController*) init
{
    if (self = [super init])
    {
        self.view = tableView = [[UITableView alloc] init];
        tableView.dataSource = self;
        tableView.delegate = self;
    }

    viewLoaded = NO;
    self.logMessages = [[NSMutableArray alloc] initWithCapacity:50];
    thisLogViewController = self;
    
    self.dateFormatter = [[NSDateFormatter alloc] init];
    [_dateFormatter setTimeStyle:NSDateFormatterMediumStyle];
    [_dateFormatter setDateStyle:NSDateFormatterNoStyle];
    
    [self appMessage:@"Log Started"];
    
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    viewLoaded = YES;
}

+ (LogViewController*) getLogViewController
{
    return thisLogViewController;
}
+ (void) logAppMessage: (NSString*) message
{
    [[LogViewController getLogViewController] appMessage:message];
}
+ (void) ClearLog
{
    [LogViewController getLogViewController].logMessages = [[NSMutableArray alloc] initWithCapacity:50];
    [(UITableView*)[LogViewController getLogViewController].view reloadData];
}
- (void)viewWillAppear:(BOOL)animated {
    if (!_logMessages)
    {
        self.logMessages = [[NSMutableArray alloc] initWithCapacity:50];
        [self appMessage:@"Log Started"];
    }
    [tableView reloadData];
    NSIndexPath *path = [NSIndexPath indexPathForRow:_logMessages.count-1 inSection:0];
    [tableView scrollToRowAtIndexPath:path atScrollPosition:UITableViewScrollPositionBottom animated:YES];
}

-(void) didConnect {
}

-(void) didDisconnect
{
}

- (void) logRobotMessage: (PubSubMsg*) message
{
    NSString *severity;
    NSString *source = @"???";
    UIImage *icon;
    SubsystemViewController *ss =
    [[MasterViewController getMasterViewController].sourceCodes objectForKey:[NSNumber numberWithInt: message.msg.header.source]];
    
    if (ss) source = ss.name;
    
    message.msg.logPayload.text[PS_MAX_LOG_TEXT] = 0;
    
    NSString *logText = [NSString stringWithFormat:@"%s", message.msg.logPayload.text];
    
    switch (message.msg.logPayload.severity){
        case SYSLOG_ROUTINE:
            severity = @"Routine";
            icon = [UIImage imageNamed:@"online.png"];
            break;
        case SYSLOG_INFO:
            severity = @"Info";
            icon = [UIImage imageNamed:@"online.png"];
            break;
        case SYSLOG_WARNING:
            severity = @"Warning";
            icon = [UIImage imageNamed:@"warning.png"];
            break;
        case SYSLOG_ERROR:
            severity = @"Error";
            icon = [UIImage imageNamed:@"fail.png"];
            break;
        case SYSLOG_FAILURE:
            severity = @"Fail";
            icon = [UIImage imageNamed:@"fail.png"];
            break;
        default:
            severity = @"???";
            icon = [UIImage imageNamed:@"offline.png"];
            break;
    }
    
    NSString *log = [NSString stringWithFormat:@"%@: %@ %@", [_dateFormatter stringFromDate:[NSDate date]], source, severity];
    
    [self logMessage: logText detail: log icon: icon];
}

- (void) logMessage: (NSString*) log detail: (NSString*) detail icon: (UIImage*) icon
{
    
    NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:
                          log, @"log",
                          detail, @"detail",
                          icon, @"icon",
                          nil];
    
    [_logMessages addObject:dict];
    
    if (viewLoaded)
    {
        NSIndexPath *path = [NSIndexPath indexPathForRow:0 inSection:0];
        [(UITableView*)self.view insertRowsAtIndexPaths:[NSArray arrayWithObject:path] withRowAnimation:UITableViewRowAnimationTop];
        path = [NSIndexPath indexPathForRow:0 inSection:0];
        [(UITableView*)self.view scrollToRowAtIndexPath:path atScrollPosition:UITableViewScrollPositionTop animated:YES];
    }
    [tableView reloadData];
}
- (void) appMessage: (NSString*) logText
{
    NSString *log = [NSString stringWithFormat:@"%@: Robo App", [_dateFormatter stringFromDate:[NSDate date]]];
    
    [self logMessage: logText detail: log icon: [UIImage imageNamed: @"street_view.png"]];
    
    NSLog(@"%@", logText);
}
-(void) didReceiveMessage: (PubSubMsg*) message
{
    switch(message.msg.header.messageType){
        case SYSLOG_MSG:
            [self logRobotMessage: message];
            break;
        default:
            break;
    }
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 1;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return _logMessages.count;
}

- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"LogViewCell"];
    
    if (!cell)
    {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"LogViewCell"];
    }
    
    NSDictionary *dict = [_logMessages objectAtIndex: (_logMessages.count - indexPath.row - 1)];
    
    cell.textLabel.text         = [dict objectForKey:@"log"];
    cell.detailTextLabel.text   = [dict objectForKey:@"detail"];
    cell.imageView.image        = [dict objectForKey:@"icon"];
    return cell;
}
- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    return nil;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
