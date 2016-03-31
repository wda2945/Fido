//
//  BehaviorController
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 5/4/15.
//  Copyright (c) 2015 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MessageDelegateProtocol.h"

@interface BehaviorViewController : UITableViewController <MessageDelegate>

@property (strong) NSMutableDictionary *availableScripts;

- (void) clearScripts;

@end