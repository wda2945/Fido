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

#define MOVE_INTERVAL 0.5f

@interface RCViewController () {
    CGPoint touchDown;
    
    NSTimer *moveTimer;
    CGPoint move, lastMove;
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
            [self sendCommandX: 0 andY: 0 ];
            break;
        case UIGestureRecognizerStateChanged:
        {
            CGPoint pan = [gr translationInView:self.view];
            [self sendCommandX: -pan.y / 10 andY: pan.x / 10];
        }
            break;
        default:
            break;
    }
}

- (void) sendCommandX: (float) x andY: (float) y {
    move.x = x;
    move.y = y;
}
- (void) moveMsg: (NSTimer*) timer
{
    if (move.x == lastMove.x && move.y == lastMove.y) return;
    
    NSLog(@"Move %f, turn %f", move.x, move.y);
    
    if ([(AppDelegate*)[[UIApplication sharedApplication] delegate] connected])
    {
        psMessage_t msg;
        msg.header.messageType = MOTORS;
        msg.motorPayload.portMotors = (move.x + move.y) * 10;
        msg.motorPayload.starboardMotors = (move.x - move.y) * 10;
        float distance = sqrt(move.x*move.x + move.y*move.y);
        msg.motorPayload.speed = (distance < 10 ? 10 : distance);
        msg.motorPayload.flags =
            (move.x>=0 ? ENABLE_FRONT_CONTACT_ABORT : 0) |
            (move.x<=0 ? ENABLE_REAR_CONTACT_ABORT : 0) |
            ENABLE_SYSTEM_ABORT;
        [PubSubMsg sendMessage:&msg];
        
        lastMove.x = move.x;
        lastMove.y = move.y;
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
