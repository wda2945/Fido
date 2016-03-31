//
//  PowerViewController.h
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 1/13/16.
//  Copyright © 2016 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "MessageDelegateProtocol.h"

extern char *powerstateNames[];

@interface PowerViewController : UITableViewController <MessageDelegate>

@property UserCommand_enum systemStateCommand;      //set system state from the user
@property PowerState_enum powerState;               //system power state from the robot
@property NSString *powerStateStr;                  //state reported by robot
- (void) newSystemState: (UserCommand_enum) s;      //set from other modules

@end
