//
//  RCViewController.m
//  Monitor
//
//  Created by Martin Lane-Smith on 4/13/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "RCViewController.h"
#import "PubSubMsg.h"
#import "AppDelegate.h"

#define MOVE_INTERVAL 0.25f

@interface RCViewController () {
    
    NSTimer *moveTimer;
    CGPoint move, lastMove;
    
    bool fingerOn;
}
- (void) moveMsg: (NSTimer*) timer;
- (void) slide: (UIPanGestureRecognizer*) gr;

@end

@implementation RCViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        move = lastMove = CGPointZero;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    fingerOn = false;
    
    UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(slide:)];
    
    pan.minimumNumberOfTouches = 1;
    pan.maximumNumberOfTouches = 2;
    
    [self.view addGestureRecognizer:pan];

    moveTimer = [NSTimer scheduledTimerWithTimeInterval:MOVE_INTERVAL
                                                 target:self
                                               selector:@selector(moveMsg:)
                                               userInfo:nil
                                                repeats:YES];
}

- (void) slide: (UIPanGestureRecognizer*) gr
{
    switch (gr.state)
    {
        case UIGestureRecognizerStateEnded:
        case UIGestureRecognizerStateCancelled:
        case UIGestureRecognizerStateFailed:
            fingerOn = false;
            break;
        case UIGestureRecognizerStateChanged:
        {
            fingerOn = true;
            
            CGPoint pan = [gr translationInView:self.view];
            move.x = -pan.y;
            move.y = pan.x;
        }
            break;
        default:
            break;
    }
}

bool lastFingerOn = false;

- (void) moveMsg: (NSTimer*) timer
{
    if ([(AppDelegate*)[[UIApplication sharedApplication] delegate] connected])
    {
        int port, starboard, speed;
        
        if (fingerOn)
        {
            if (fabs(move.y) > fabs(move.x))
            {
                if (move.y > 0)
                {
                    port = 30;
                    starboard = -30;
                    speed = (int) fabs(move.y);
                }
                else
                {
                    port = -30;
                    starboard = 30;
                    speed = (int) fabs(move.y);
                }
            }
            else
            {
                if (move.x > 0)
                {
                    port = starboard = 30;
                    speed = (int) fabs(move.x);
                }
                else
                {
                    port = starboard = -30;
                    speed = (int) fabs(move.x);
                }
            }
        }
        else
        {
            if (lastFingerOn) {
                port = starboard = speed = 0;
            }
            else{
                lastFingerOn = false;
                return;
            }
        }
        
        psMessage_t msg;
        msg.header.messageType = MOTORS;
        msg.motorPayload.portMotors = port;
        msg.motorPayload.starboardMotors = starboard;
        msg.motorPayload.speed = speed;
        msg.motorPayload.flags =
            (move.x>=0 ? ENABLE_FRONT_CONTACT_ABORT : 0) |
            (move.x<=0 ? ENABLE_REAR_CONTACT_ABORT : 0) |
            ENABLE_SYSTEM_ABORT;
        [PubSubMsg sendMessage:&msg];
        
        NSLog(@"Motors: port %i, starboard %i, speed %i", port, starboard, speed);

    }
}
-(void) didConnect{
}
-(void) didDisconnect{
}
-(void) didReceiveMessage: (PubSubMsg*) message
{
    
}
- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
