//
//  ConditionsViewController.h
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 1/13/16.
//  Copyright © 2016 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "MessageDelegateProtocol.h"

@interface ConditionsViewController : UITableViewController <MessageDelegate>

- (void) clearConditions;

@end
