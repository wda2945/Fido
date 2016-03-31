//
//  AppDelegate.h
//  Monitor
//
//  Created by Martin Lane-Smith on 4/12/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "BLE.h"
#import "PubSubMsg.h"
#import "MessageDelegateProtocol.h"
#import "MasterViewController.h"
#import "CollectionViewController.h"
#import <CoreFoundation/CoreFoundation.h>

typedef enum {FIDO_XBEE_SERVER, FIDO_OVM_SERVER, BLE_SERVER, SERVER_COUNT} Server_enum;
#define INET_SERVER_COUNT 2

typedef enum {SERVER_OFFLINE, SERVER_CONNECTED, SERVER_ONLINE} Status_enum;

@interface AppDelegate : UIResponder <UIApplicationDelegate, BLEDelegate, NSNetServiceDelegate, NSNetServiceBrowserDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) UISplitViewController *splitViewController;
@property (strong, nonatomic) CollectionViewController *collectionController;
@property (strong, nonatomic) BLE *BLEobject;

@property (strong, nonatomic) NSMutableArray *msgQueue;

@property struct timeval pingTime;

@property  (readonly) bool bleConnected;
@property  (readonly) bool connected;

- (void) sendMessage: (PubSubMsg*) msg;

- (void) enableComms: (int) server enabled: (BOOL) state;

//stats
- (long int) MessagesSent: (int) server;
- (long int) MessagesReceived: (int) server;
- (long int) SendErrors: (int) server;
- (long int) ReceiveErrors: (int) server;

extern bool socketConnected[INET_SERVER_COUNT];             //sockets valid
extern bool keepaliveReceived[INET_SERVER_COUNT];
extern bool bleKeepaliveReceived;

extern char *serverNames[INET_SERVER_COUNT];

/* Subtract the ‘struct timeval’ values X and Y,
 storing the result in RESULT.
 Return 1 if the difference is negative, otherwise 0. */
int timeval_subtract (struct timeval *result,struct timeval  *x,struct timeval  *y);

@end
