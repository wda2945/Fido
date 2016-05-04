//
//  ConditionsViewController.h
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 1/13/16.
//  Copyright © 2016 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "MessageDelegateProtocol.h"

typedef enum {CONDITIONS_ERRORS, CONDITIONS_PROXIMITY, CONDITIONS_STATUS} ConditionsList_enum;

@interface ConditionsViewController : UITableViewController <MessageDelegate>

- (ConditionsViewController*) initForList: (ConditionsList_enum) list;

- (bool) isConditionSet: (Condition_enum) c;

@end
