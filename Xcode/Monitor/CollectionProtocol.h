//
//  CollectionProtocol.h
//  FidoMonitor
//
//  Created by Martin Lane-Smith on 2/2/16.
//  Copyright © 2016 Martin Lane-Smith. All rights reserved.
//

#ifndef CollectionProtocol_h
#define CollectionProtocol_h

#import "SubsystemViewController.h"

@protocol CollectionController
@required
- (bool) presentView: (NSString*) name;
- (void) addSubsystem: (SubsystemViewController*) sub;
@end

#endif /* CollectionProtocol_h */
