//
//  SettingsDetailViewController.m
//  Monitor
//
//  Created by Martin Lane-Smith on 4/12/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#import "SettingsDetailViewController.h"
#import "LogViewController.h"
#import "PubSubMsg.h"

@interface SettingsDetailViewController ()
{
    float fMinimum, fMaximum, fCurrent, fNewValue;
    int iMinimum, iMaximum, iCurrent, iNewValue;
    int settingNumber;
    bool intSetting;
    NSString *name;
    NSMutableDictionary * settingsDict;
}
@property (weak, nonatomic) IBOutlet UILabel *lblTitle;
@property (weak, nonatomic) IBOutlet UILabel *lblCurrent;
@property (weak, nonatomic) IBOutlet UILabel *lblNew;
@property (weak, nonatomic) IBOutlet UILabel *lblMinimum;
@property (weak, nonatomic) IBOutlet UILabel *lblMaximum;
@property (weak, nonatomic) IBOutlet UILabel *lblOffline;
- (IBAction)sliderChanged:(UISlider *)sender;
@property (weak, nonatomic) IBOutlet UISlider *slider;
- (IBAction)btnSend:(UIButton *)sender;

@end

@implementation SettingsDetailViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void) settingsDict: (NSMutableDictionary *) dict{
    settingsDict = dict;
    name            = [dict objectForKey:@"name"];
    intSetting      = [(NSNumber*)[dict objectForKey:@"integer"] boolValue];
    if (intSetting)
    {
        iMinimum         = [(NSNumber*)[dict objectForKey:@"min"] intValue];
        iMaximum         = [(NSNumber*)[dict objectForKey:@"max"] intValue];
        iCurrent         = [(NSNumber*)[dict objectForKey:@"value"] intValue];
        iNewValue        = iCurrent;
    }
    else
    {
        fMinimum         = [(NSNumber*)[dict objectForKey:@"min"] floatValue];
        fMaximum         = [(NSNumber*)[dict objectForKey:@"max"] floatValue];
        fCurrent         = [(NSNumber*)[dict objectForKey:@"value"] floatValue];
        fNewValue        = fCurrent;
    }
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
}
- (void)viewWillAppear:(BOOL)animated {
    _lblTitle.text = name;
    if (intSetting)
    {
        _slider.minimumValue = iMinimum;
        _slider.maximumValue = iMaximum;
        _slider.value = iCurrent;
        
        _lblMinimum.text = [NSString stringWithFormat:@"%i",iMinimum];
        _lblMaximum.text = [NSString stringWithFormat:@"%i",iMaximum];
        _lblCurrent.text = [NSString stringWithFormat:@"Current Value: %i",iCurrent];
        _lblNew.text = [NSString stringWithFormat:@"New Value: %i",iNewValue];
    }
    else
    {
        _slider.minimumValue = fMinimum;
        _slider.maximumValue = fMaximum;
        _slider.value = fCurrent;
        
        if (fMaximum > 100){
            _lblMinimum.text = [NSString stringWithFormat:@"%.0f",fMinimum];
            _lblMaximum.text = [NSString stringWithFormat:@"%.0f",fMaximum];
            _lblCurrent.text = [NSString stringWithFormat:@"Current Value: %.0f",fCurrent];
            _lblNew.text = [NSString stringWithFormat:@"New Value: %.0f",fNewValue];
        }else if (fMaximum >1){
            _lblMinimum.text = [NSString stringWithFormat:@"%.1f",fMinimum];
            _lblMaximum.text = [NSString stringWithFormat:@"%.1f",fMaximum];
            _lblCurrent.text = [NSString stringWithFormat:@"Current Value: %.1f",fCurrent];
            _lblNew.text = [NSString stringWithFormat:@"New Value: %.1f",fNewValue];
        }
        else if (fMaximum > 0.1f){
            _lblMinimum.text = [NSString stringWithFormat:@"%.2f",fMinimum];
            _lblMaximum.text = [NSString stringWithFormat:@"%.2f",fMaximum];
            _lblCurrent.text = [NSString stringWithFormat:@"Current Value: %.2f",fCurrent];
            _lblNew.text = [NSString stringWithFormat:@"New Value: %.2f",fNewValue];
        } else{
            _lblMinimum.text = [NSString stringWithFormat:@"%.3f",fMinimum];
            _lblMaximum.text = [NSString stringWithFormat:@"%.3f",fMaximum];
            _lblCurrent.text = [NSString stringWithFormat:@"Current Value: %.3f",fCurrent];
            _lblNew.text = [NSString stringWithFormat:@"New Value: %.3f",fNewValue];
        }
    }
    [super viewWillAppear:animated];
}
- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)sliderChanged:(UISlider *)sender {
    if (intSetting)
    {
        iNewValue = sender.value;
        _lblNew.text = [NSString stringWithFormat:@"New Value: %i",iNewValue];
    }
    else
    {
        fNewValue = sender.value;
        if (fMaximum > 100){
            _lblNew.text = [NSString stringWithFormat:@"New Value: %.0f",fNewValue];
        }else if (fMaximum >1){
            _lblNew.text = [NSString stringWithFormat:@"New Value: %.1f",fNewValue];
        }
        else if (fMaximum > 0.1f){
            _lblNew.text = [NSString stringWithFormat:@"New Value: %.2f",fNewValue];
        } else{
            _lblNew.text = [NSString stringWithFormat:@"New Value: %.3f",fNewValue];
        }
    }
}
- (IBAction)btnSend:(UIButton *)sender {
    psMessage_t msg;
    
    if (intSetting)
    {
        [name getCString:msg.optionPayload.name maxLength:PS_NAME_LENGTH encoding:NSASCIIStringEncoding];
        
        msg.header.messageType = SET_OPTION;
        msg.optionPayload.value = iNewValue;
        
        if ([PubSubMsg sendMessage:&msg]){
            iCurrent = iNewValue;
            _lblCurrent.text = [NSString stringWithFormat:@"Current Value: %i",iCurrent];
            
            [settingsDict setObject:[NSNumber numberWithInt: iCurrent] forKey:@"value"];
            [LogViewController logAppMessage:[NSString stringWithFormat:@"%@ = %i", name, iNewValue]];
        }
    }
    else
    {
        [name getCString:msg.settingPayload.name maxLength:PS_NAME_LENGTH encoding:NSASCIIStringEncoding];
        
        msg.header.messageType = NEW_SETTING;
        msg.settingPayload.value = fNewValue;
        
        if ([PubSubMsg sendMessage:&msg]){
            fCurrent = fNewValue;
            if (fMaximum > 100){
                _lblCurrent.text = [NSString stringWithFormat:@"Current Value: %.0f",fCurrent];
            }else if (fMaximum >1){
                _lblCurrent.text = [NSString stringWithFormat:@"Current Value: %.1f",fCurrent];
            }
            else if (fMaximum > 0.1f){
                _lblCurrent.text = [NSString stringWithFormat:@"Current Value: %.2f",fCurrent];
            } else{
                _lblCurrent.text = [NSString stringWithFormat:@"Current Value: %.3f",fCurrent];
            }
            [settingsDict setObject:[NSNumber numberWithFloat:fCurrent] forKey:@"value"];
            
            [LogViewController logAppMessage:[NSString stringWithFormat:@"%@ = %f", name, fNewValue]];
        }
    }
    
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
    {
        [self dismissViewControllerAnimated:YES completion:nil];
    }
    else {
        [self.navigationController popViewControllerAnimated: YES];
    }
}


@end
