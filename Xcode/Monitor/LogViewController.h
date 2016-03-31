//
//  DetailViewController.h
//  Monitor
//
//  Created by Martin Lane-Smith on 4/12/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MessageDelegateProtocol.h"

@interface LogViewController : UITableViewController < MessageDelegate>

+ (LogViewController*) getLogViewController;
+ (void) logAppMessage: (NSString*) message;
+ (void) ClearLog;

@end
