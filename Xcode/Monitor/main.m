//
//  main.m
//  Monitor
//
//  Created by Martin Lane-Smith on 4/12/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "AppDelegate.h"

void fatal_error_signal (int sig);

int main(int argc, char * argv[])
{
    signal(SIGILL, fatal_error_signal);
    signal(SIGABRT, fatal_error_signal);
    signal(SIGIOT, fatal_error_signal);
    signal(SIGBUS, fatal_error_signal);
    signal(SIGFPE, fatal_error_signal);
    signal(SIGSEGV, fatal_error_signal);
    signal(SIGTERM, fatal_error_signal);
    signal(SIGCHLD, fatal_error_signal);
    signal(SIGSYS, fatal_error_signal);
    signal(SIGCHLD, fatal_error_signal);
    
    
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

//other signals
volatile sig_atomic_t fatal_error_in_progress = 0;
void fatal_error_signal (int sig)
{
    /* Since this handler is established for more than one kind of signal, it might still get invoked recursively by delivery of some other kind of signal. Use a static variable to keep track of that. */
    if (fatal_error_in_progress)
        raise (sig);
    fatal_error_in_progress = 1;
    
    NSLog(@"Signal %i raised", sig);
    
    sleep(1);	//let there be printing
    
    /* Now re-raise the signal. We reactivate the signal�s default handling, which is to terminate the process. We could just call exit or abort,
     but re-raising the signal sets the return status
     from the process correctly. */
    signal (sig, SIG_DFL);
    raise (sig);
}