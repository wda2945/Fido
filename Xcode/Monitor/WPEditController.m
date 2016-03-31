//
//  WPiPhoneController.m
//  RoboMonitor
//
//  Created by Martin Lane-Smith on 11/27/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "WPEditController.h"

@interface WPEditController ()
@property (weak, nonatomic) IBOutlet UILabel *lblViewTitle;
@property (weak, nonatomic) IBOutlet UITextField *txtWPName;
@property (weak, nonatomic) IBOutlet UILabel *lblLocation;
- (IBAction)editingDidEnd:(id)sender;
- (IBAction)btnOK:(id)sender;
- (IBAction)btnCancel:(id)sender;

@end

@implementation WPEditController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
}

- (void) initPanel: (NSString*) title wpName: (NSString*) name location: (CLLocation*) loc
{
    
    self.lblViewTitle.text = title;
    self.txtWPName.text = name;
    
    self.lblLocation.text = [NSString stringWithFormat:@"%f, %f", loc.coordinate.latitude, loc.coordinate.longitude];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)editingDidEnd:(id)sender {
    
}
- (BOOL) textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return NO;
}
- (IBAction)btnOK:(id)sender {
    
    [_delegate waypointName: _txtWPName.text cancelled: NO];
}

- (IBAction)btnCancel:(id)sender {
    [_delegate waypointName: _txtWPName.text cancelled: YES];
}
@end
