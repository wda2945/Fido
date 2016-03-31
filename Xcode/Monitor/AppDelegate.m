//
//  AppDelegate.m
//  Monitor
//
//  Created by Martin Lane-Smith on 4/12/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "AppDelegate.h"

#import "Helpers.h"
#import "pubsubparser.h"


#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT_NUMBER 50000
#define PING_INTERVAL 10.0f


@interface AppDelegate () {
    
    NSString *captions[SERVER_COUNT];
    
    dispatch_queue_t recvQueue[INET_SERVER_COUNT];         //GCD Queue for recv
    dispatch_queue_t sendQueue;         //GCD Queue for send
    
    uint8_t msgSequence[INET_SERVER_COUNT];
    uint8_t checksum[INET_SERVER_COUNT];
    int errorreply[INET_SERVER_COUNT];

    NSTimer *pingTimer;
    
    //BLE
    char encodedMessage[512];
    int bytesReceived;
    char *nextByte;
    
    bool currentConnected;
    
    bool channelEnabled[SERVER_COUNT];
    long int messagesSent[SERVER_COUNT];
    long int messagesReceived[SERVER_COUNT];
    long int receiveErrors[SERVER_COUNT];
    long int sendErrors[SERVER_COUNT];
}

- (void) BLEStartScan;

@end

@implementation AppDelegate

int sendSocket[INET_SERVER_COUNT];                   //FD for server write
int recvSocket[INET_SERVER_COUNT];                   //FD for server read

bool socketConnecting[INET_SERVER_COUNT];
bool socketConnected[INET_SERVER_COUNT];             //sockets valid

bool keepaliveReceived[INET_SERVER_COUNT];
int keepaliveCountdown[INET_SERVER_COUNT];
struct timeval keepaliveTime[INET_SERVER_COUNT];

bool bleKeepaliveReceived;
int bleKeepaliveCountdown;
struct timeval bleKeepaliveTime;

#define KEEPALIVE_COUNTDOWN 5

typedef union {
    uint8_t bytes[4];
    in_addr_t address;
} IPaddress_t;

 char *serverNames[INET_SERVER_COUNT] = {
    "fido_proxy.local",
     "fido.local",
};

#define ADDRESS_COUNT 2

int connectionList[INET_SERVER_COUNT][ADDRESS_COUNT] = {
    {219, 0},           //fido_proxy.local
    {169, 28},        //fido.local aka x.x.x.169 & x.x.x.28
};

//int activeConnection = -1;

- (bool) connected
{
    for (int i=0; i<SERVER_COUNT; i++)
    {
        if (socketConnected[i] && channelEnabled[i]) return YES;
    }
    return NO;
}

- (bool) anyConnected
{
    if ((_bleConnected && channelEnabled[BLE_SERVER]) || [self connected]) return YES;
    else return NO;
}

void SIGPIPEhandler(int sig);
void InstallSignalHandler();

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    currentConnected = NO;
    
    // Override point for customization after application launch.
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        self.splitViewController = (UISplitViewController *)self.window.rootViewController;
        self.collectionController  = [_splitViewController.viewControllers lastObject];
        _splitViewController.delegate = _collectionController;
    }
    
    //set SIG handler
    InstallSignalHandler();
    NSLog(@"SIGPIPE handler set");

    for (int i=0; i<SERVER_COUNT; i++)
    {
        recvQueue[i] = dispatch_queue_create (NULL, NULL);
        if (i == FIDO_XBEE_SERVER)
        {
            channelEnabled[i] = YES;
            
        }
        else
        {
            channelEnabled[i] = NO;
        }
        [self setConnectedCaption: @"Searching..." forServer:i connected:NO];
    }
    
    sendQueue = dispatch_queue_create (NULL, NULL);
    
    pingTimer = [NSTimer scheduledTimerWithTimeInterval:PING_INTERVAL
                                                 target:self
                                               selector:@selector(ping:)
                                               userInfo:nil
                                                repeats:YES];
    [self ping: nil];
    
    //BLE
    _bleConnected = NO;
    self.BLEobject = [[BLE alloc] init];
    [self.BLEobject controlSetup];
    self.BLEobject.delegate = self;
    [self BLEStartScan];
    
    nextByte = encodedMessage;
    bytesReceived = 0;
    
    self.msgQueue = [NSMutableArray array];
    
    return YES;
}

- (void) setConnectedCaption: (NSString *) caption forServer: (int) index  connected: (bool) conn
{
    captions[index] = caption;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        if (channelEnabled[index])
        {
            [[MasterViewController getMasterViewController] setConnectionCaption: caption forServer: index  connected: conn];
        }
        else{
            [[MasterViewController getMasterViewController] setConnectionCaption: @"Disabled" forServer: index  connected: conn];
        }
     });
}

- (void) enableComms: (int) index enabled: (BOOL) state
{
    if (index < SERVER_COUNT)
    {
        channelEnabled[index] = state;
        
        if (state)
        {
            [[MasterViewController getMasterViewController] setConnectionCaption: captions[index] forServer:  index connected: (index < BLE_SERVER ? socketConnected[index] : _bleConnected)];
        }
        else{
            [[MasterViewController getMasterViewController] setConnectionCaption: @"Disabled" forServer:  index connected: (index < BLE_SERVER ? socketConnected[index] : _bleConnected)];
        }
    }
}

//--------------------------Socket stuff

//convenience function
- (void) write: (uint8_t) c toSocket: (int) index
{
    size_t reply = send(sendSocket[index], &c, 1, 0);
    if (reply == 1)
    {
        checksum[index] += c;
    }
    else
    {
        errorreply[index] = -1;
    }
}

//sends a message to the robot
//runs on sendQueue thread

- (void) sendUsingChannel: (int) socketIndex message: (PubSubMsg*) message
{
    psMessage_t txMessage;
    [message copyMessage: &txMessage];
    uint8_t size;
    uint8_t *buffer;
    
    errorreply[socketIndex] = 0;
    
    //sending binary
    //send STX
    [self write:  STX_CHAR toSocket: socketIndex];
    checksum[socketIndex] = 0; //checksum starts from here
    //send header
    [self write:  txMessage.header.length toSocket: socketIndex];
    [self write:  ~txMessage.header.length toSocket: socketIndex];
    [self write:  msgSequence[socketIndex]++ toSocket: socketIndex];
    [self write:  txMessage.header.source toSocket: socketIndex];
    [self write:  txMessage.header.messageType toSocket: socketIndex];
    //send payload
    buffer = (uint8_t *) &txMessage.packet;
    size = txMessage.header.length;
    
    if (size > sizeof(psMessage_t) - SIZEOF_HEADER)
    {
        size = txMessage.header.length = sizeof(psMessage_t) - SIZEOF_HEADER;
    }
    
    while (size) {
        [self write:  *buffer toSocket: socketIndex];
        buffer++;
        size--;
    }
    //write checksum
    [self write:  (checksum[socketIndex] & 0xff) toSocket: socketIndex];
    
    if (errorreply < 0)
    {
        NSLog(@"send error: %s\n", strerror(errno));
        socketConnected[socketIndex] = false;
        sendErrors[socketIndex]++;
    }
    else{
        messagesSent[socketIndex]++;
        
        NSLog(@"%s sent via %s", psLongMsgNames[txMessage.header.messageType], serverNames[socketIndex] );
    }
}

- (void) sendToServer: (PubSubMsg *)message
{
    for (int i=0; i<INET_SERVER_COUNT; i++)
    {
        if (socketConnected[i] && channelEnabled[i])
        {
            [self sendUsingChannel: i message: message];
            break;
        }
    }
}

//called to send a message
//transfers to sendQueue thread
- (void) sendMessage: (PubSubMsg *)message
{
    if ([self connected])
    {
        //we have a socket connected
        //dispatch msg to sendQueue
        dispatch_async(sendQueue, ^{
            [self sendToServer: message];
        });
    }
    else
    {
        if (channelEnabled[BLE_SERVER])
        {
            //use BLE
            [_msgQueue addObject:message];
        }
        else{
            [self setConnectedCaption: @"Disabled" forServer: BLE_SERVER connected: _bleConnected];
        }
    }
}

//called on readQueue to service messages from the robot
//exits when the connection fails
- (void) readFromServer: (int) index
{
    uint8_t readByte;
    int result;
    
    psMessage_t rxMessage;
    status_t parseStatus;
    
    parseStatus.noCRC       = 0; ///< Do not expect a CRC, if > 0
    parseStatus.noSeq       = 0; ///< Do not check seq #s, if > 0
    parseStatus.noLength2   = 0; ///< Do not check for duplicate length, if > 0
    parseStatus.noTopic     = 1; ///< Do not check for topic ID, if > 0
    ResetParseStatus(&parseStatus);
    
    do {        //while connected
        
        do {    //while parsing message
            if (recv(recvSocket[index], &readByte, 1, 0) <= 0)
            {
                //quit on failure, EOF, etc.
                NSLog(@"%s recv() error: %s\n", serverNames[index], strerror(errno));
                receiveErrors[index]++;
                return;
            }
            result = ParseNextCharacter(readByte, &rxMessage, &parseStatus);
        } while (result == 0);
        
        PubSubMsg *psMessage = [[PubSubMsg alloc] initWithMsg: &rxMessage];
        
        if (psMessage)
        {
            NSLog(@"%s received via %s", psLongMsgNames[rxMessage.header.messageType], serverNames[index]);
            
            //dispatch msg to main queue
            dispatch_async(dispatch_get_main_queue(), ^{
                if (psMessage.msg.header.messageType == KEEPALIVE)
                {
                    struct timeval pingTime, responseTime, interval;
                    float latency;
                    
                    keepaliveReceived[index] = true;
                    
                    pingTime = keepaliveTime[index];
                    gettimeofday(&responseTime, NULL);
                    
                    if (!timeval_subtract(&interval, &responseTime, &pingTime))
                    {
                        latency = interval.tv_sec * 1000 + interval.tv_usec / 1000;
                    }
                    else{
                        latency = -1;
                    }
                    [[MasterViewController getMasterViewController].commsViewController newLatencyMeasure: latency forServer: index];
                }
                else
                {
                    [[MasterViewController getMasterViewController] didReceiveMessage: psMessage];
                    messagesReceived[index]++;
                }
            });
        }
        else
        {
            NSLog(@"Bad message from %s", serverNames[index]);
            receiveErrors[index]++;
        }
        
    } while (socketConnected[index]);
    //read fail or disconnect
}

#define MAXSLEEP 128
int connect_retry(int domain, int type, int protocol, const struct sockaddr *addr, socklen_t alen)
{
    int numsec, fd;
    
    for (numsec = 1; numsec < MAXSLEEP; numsec <<= 1)
    {
        if ((fd = socket(domain, type, protocol)) < 0)
        {
            return -1;
        }
        if (connect(fd, addr, alen) == 0)
        {
            //accepted
            return fd;
        }
        close(fd);
        
        //delay
        if (numsec <= MAXSLEEP/2)
            sleep(numsec);
    }
    return(-1);
}

typedef enum {CONNECT_ERROR, LOST_CONNECTION} ServerConnectResult_enum;

- (ServerConnectResult_enum) connectTo: (int) index atAddress: (IPaddress_t) ipAddress
{
    char *serverName = serverNames[index];
    
    struct sockaddr_in serverSockAddress;
    
    NSString *IP = [NSString stringWithFormat: @"%i.%i.%i.%i", ipAddress.bytes[0], ipAddress.bytes[1], ipAddress.bytes[2], ipAddress.bytes[3]];
    
    [self setConnectedCaption: IP forServer: index connected: NO];
    [LogViewController logAppMessage: [NSString stringWithFormat: @"%s: %@", serverName, IP]];

    memset(&serverSockAddress, 0, sizeof(serverSockAddress));
    serverSockAddress.sin_len = sizeof(serverSockAddress);
    serverSockAddress.sin_family = AF_INET;
    serverSockAddress.sin_port = htons(PORT_NUMBER);
    serverSockAddress.sin_addr.s_addr = ipAddress.address;
    
    //create socket & connect
    recvSocket[index] = connect_retry(AF_INET, SOCK_STREAM, 0, (const struct sockaddr*) &serverSockAddress, sizeof(serverSockAddress));
    
    if (recvSocket[index] < 0)
    {
        [self setConnectedCaption: @"connect() error" forServer: index connected: NO];
        [LogViewController logAppMessage: [NSString stringWithFormat: @"%s connect() error: %s", serverName, strerror(errno)]];
        socketConnecting[index] = false;
        return CONNECT_ERROR;
    }
    else
    {
        [self setConnectedCaption: IP forServer: index connected: YES];
        NSLog(@"%s @ %@", serverName, IP);
        
        //dup a socket for the send thread
        sendSocket[index] = dup(recvSocket[index]);
        
        socketConnected[index]  = true;
        socketConnecting[index] = false;
       
        //read from server loops until the pipe fails
        [self readFromServer: index];
        
        close(recvSocket[index]);
        
        //use GCD to process close() on send queue - between messages
        dispatch_sync(sendQueue, ^{
            close(sendSocket[index]);
            socketConnected[index] = false;
        });
        [self setConnectedCaption: @"Connection lost" forServer: index connected: NO];
        [LogViewController logAppMessage: [NSString stringWithFormat: @"%s connection lost", serverName]];
        
        return LOST_CONNECTION;
    }
}

//called by ping timer periodically to try to connect to the robot using the readQueue thread
//if successful, it continues to receive message
//if/when unsuccessful, it exits, to be called again at the next timer fire
- (void) connectToServer: (int) index
{
    char *serverName = serverNames[index];
    
    struct hostent *server;
    IPaddress_t ipAddress;
    
    //First try Bonjour name
    
    [self setConnectedCaption: @"Searching..." forServer: index connected: NO];
    
    server = gethostbyname2(serverName, AF_INET);
    if (server != NULL)
    {
        memcpy(ipAddress.bytes, server->h_addr, 4);
        if ([self connectTo: index atAddress: ipAddress] == LOST_CONNECTION)
        {
            return;
        }
    }
    
    //then try some alternatives
    ipAddress.bytes[0] = 192;
    ipAddress.bytes[1] = 168;
    ipAddress.bytes[2] = 1;
    
    for (int i=0; i<ADDRESS_COUNT; i++)
    {
        ipAddress.bytes[3] = connectionList[index][i];
        
        if (ipAddress.bytes[3] != 0)
        {
            if ([self connectTo: index atAddress: ipAddress] == LOST_CONNECTION)
            {
                return;
            }
        }
    }
    
}

-(void) ping:(NSTimer*) timer{

    //look for a connection
    for (int i=0; i<INET_SERVER_COUNT; i++)
    {
        if (!socketConnected[i] && !socketConnecting[i] && channelEnabled[i])
        {
            socketConnecting[i] = true;
            keepaliveReceived[i] = true;    //start with it set
            keepaliveCountdown[i] = KEEPALIVE_COUNTDOWN;
            
            dispatch_async(recvQueue[i], ^{
                [self connectToServer: i ];
            });
            break;
        }
    }
    
    //send a keepalive message on each open channel
    PubSubMsg *keepaliveMsg;
    
    for (int i=0; i<INET_SERVER_COUNT; i++)
    {
        if (socketConnected[i])
        {
            if (!keepaliveReceived[i])
            {
                if (--keepaliveCountdown[i] <= 0)
                {
//                    //timed out
//                    socketConnected[i] = false;
//                    close(recvSocket[i]);
//                    close(sendSocket[i]);
//                    [self setConnectedCaption: @"Timed out" forServer: i connected: NO];
//                    [LogViewController logAppMessage: [NSString stringWithFormat: @"%s timed out", serverNames[i]]];
                }
            }
            else
            {
                keepaliveCountdown[i] = KEEPALIVE_COUNTDOWN;
                keepaliveMsg = [PubSubMsg keepAlive];
                
                keepaliveReceived[i] = false;
                
                //send another keepalive
                dispatch_async(sendQueue, ^{
                    [self sendUsingChannel: i message: keepaliveMsg];
                });
                
                gettimeofday(&keepaliveTime[i], NULL);
            }
        }
    }
    
    //keepalive for BLE
    if (_bleConnected && channelEnabled[BLE_SERVER])
    {
        if (!bleKeepaliveReceived)
        {
            if (--bleKeepaliveCountdown <= 0)
            {
//                //timed out
//                [self setConnectedCaption: @"Timing out" forServer: BLE_SERVER connected: YES];
//                [LogViewController logAppMessage: @"BLE Timing out"];
            }
        }
        else
        {
            bleKeepaliveCountdown = KEEPALIVE_COUNTDOWN;
            [self setConnectedCaption: @"Connected" forServer: BLE_SERVER connected: YES];
        }
        
        keepaliveMsg = [PubSubMsg keepAlive];
        
        bleKeepaliveReceived = false;
        //send another keepalive
        [_msgQueue addObject:keepaliveMsg];
        gettimeofday(&bleKeepaliveTime, NULL);
        
    }
    
    bool nowConnected = [self connected];
    
    if (nowConnected && !currentConnected)
    {
        [[MasterViewController getMasterViewController] didConnect];
    }

    if (!nowConnected && currentConnected)
    {
        [[MasterViewController getMasterViewController] didDisconnect];
    }

    currentConnected = nowConnected;
 }

void SIGPIPEhandler(int sig)
{
    for (int i=0; i<INET_SERVER_COUNT; i++)
    {
        if (socketConnected[i])
        {
            socketConnected[i] = false;
            close(recvSocket[i]);
            close(sendSocket[i]);
        }
    }
    [LogViewController logAppMessage: @"SIGPIPE"];
}

- (void) agentOnline: (bool) tf
{
}
//--------------------------BLE Stuff

NSTimer *rssiTimer;
NSTimer *txTimer;

-(void) readRSSITimer:(NSTimer *)timer
{
    [_BLEobject readRSSI];
}
-(void) bleDidUpdateRSSI:(NSNumber *) rssi
{
    [self setConnectedCaption: [NSString stringWithFormat:@"Connected: %.0f dB", [rssi floatValue]] forServer: BLE_SERVER connected: YES];
}
-(void) txTimer:(NSTimer *)timer
{
    if (self.msgQueue.count > 0)
    {
        if (_bleConnected && channelEnabled[BLE_SERVER])
        {
            PubSubMsg *next = [self.msgQueue objectAtIndex: 0];
            if ([next sendPart] == NO)
            {
                //done
                NSLog(@"Tx: %s", psLongMsgNames[next.msg.header.messageType]);

                [self.msgQueue removeObjectAtIndex:0];
                messagesSent[BLE_SERVER]++;
            }
        }
        else{
            [self.msgQueue removeAllObjects];
        }
    }
}
-(void) didConnect
{
    if (!_bleConnected) {
        _bleConnected = YES;
    
        // Schedule to read RSSI every 1 sec.
        rssiTimer = [NSTimer scheduledTimerWithTimeInterval:(float)1.0 target:self selector:@selector(readRSSITimer:) userInfo:nil repeats:YES];
        
        // Schedule to transmit every 300mS.
        txTimer = [NSTimer scheduledTimerWithTimeInterval:(float)0.3f target:self selector:@selector(txTimer:) userInfo:nil repeats:YES];
    
        [self setConnectedCaption: @"Connected" forServer: BLE_SERVER connected: YES];
        [LogViewController logAppMessage:@"BLE did connect"];
    }
}
-(void) didDisconnect
{
    if (_bleConnected) {
        _bleConnected = NO;
        [self setConnectedCaption: @"Searching" forServer: BLE_SERVER connected: NO];
        [LogViewController logAppMessage:@"BLE did disconnect"];
    }
    [self BLEStartScan];
}

-(void) bleDidReceiveData:(unsigned char *) data length:(int) length
{
    if (!_bleConnected){
        [self didConnect];
    }
    
    if (length + bytesReceived > 512) {
        bytesReceived = 0;
        nextByte = encodedMessage;  //too much data ignore it all
    }
    else {
        do {
            char next = *data++;
            length--;
            
            switch(next) {
                default:
                    *nextByte++ = next;
                    bytesReceived++;
                    break;
                case '!':
                    *nextByte = '\0';
                {
                    PubSubMsg *message = [[PubSubMsg alloc] initWithData: encodedMessage];
                    
                    if (message) {
                        if (message.msg.header.messageType == KEEPALIVE)
                        {
                            struct timeval pingTime, responseTime, interval;
                            float latency;
                            
                            bleKeepaliveReceived = true;
                            
                            pingTime = bleKeepaliveTime;
                            gettimeofday(&responseTime, NULL);
                            
                            if (!timeval_subtract(&interval, &responseTime, &pingTime))
                            {
                                latency = interval.tv_sec * 1000 + interval.tv_usec / 1000;
                            }
                            else{
                                latency = -1;
                            }
                            [[MasterViewController getMasterViewController].commsViewController newLatencyMeasure: latency forServer: BLE_SERVER];
                        }
                        else
                        {
                            [[MasterViewController getMasterViewController] didReceiveMessage: message];
                            messagesReceived[BLE_SERVER]++;
                        }
                    }
                    bytesReceived = 0;
                    nextByte = encodedMessage;
                }
                    break;
                case '\r':  //ignore
                case '\n':
                case 0:
                    break;
            }
        }
        while (length);
    }
}

- (void) BLEStartScan{
    
    if (self.BLEobject.activePeripheral)
        if(self.BLEobject.activePeripheral.state == CBPeripheralStateConnected){
            [self didConnect];
            return;
        }

    [self setConnectedCaption: @"Scanning.."forServer: BLE_SERVER connected: _bleConnected];

    if (self.BLEobject.peripherals)
        self.BLEobject.peripherals = nil;
    
    [self.BLEobject findBLEPeripherals:3];
    
    [NSTimer scheduledTimerWithTimeInterval:(float)3.0 target:self selector:@selector(connectionTimer:) userInfo:nil repeats:NO];
}
// Called when scan period is over to connect to the first found peripheral
-(void) connectionTimer:(NSTimer *)timer
{
    if(self.BLEobject.peripherals.count > 0)
    {
        [self.BLEobject connectPeripheral:[self.BLEobject.peripherals objectAtIndex:0]];
        [self setConnectedCaption: @"Connected"forServer: BLE_SERVER connected: YES];
    }
    else
    {
        [self didDisconnect];
    }
}

//-----------------Boilerplate

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

void InstallSignalHandler()
{
    // Make sure the signal does not terminate the application.
    signal(SIGPIPE, SIG_IGN);
    
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGPIPE, 0, queue);
    
    if (source)
    {
        dispatch_source_set_event_handler(source, ^{
            SIGPIPEhandler(SIGPIPE);
        });
        
        // Start processing signals
        dispatch_resume(source);
    }
}

//stats
- (long int) MessagesSent: (int) server
{
    return messagesSent[server];
}
- (long int) MessagesReceived: (int) server
{
    return messagesReceived[server];
}
- (long int) SendErrors: (int) server
{
    return sendErrors[server];
}
- (long int) ReceiveErrors: (int) server
{
    return receiveErrors[server];
}
@end
