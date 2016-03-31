//
//  PPSsetup.c
//  PPSconfig
//
//  Created by Martin Lane-Smith on 4/22/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include "HardwareProfile.h"
#include "plib.h"

//#include "ChipKitDP32PPS.h"
#include "FubarinoMiniPPS.h"

int SetPPS(char *_pinName, char *signalName)
{
    int p, grp, i;
    //find pin #
    int pinNumber = -1;
    for (p = 0; p < PIN_COUNT; p++)
    {
        if (strcmp(pinName[p], _pinName) == 0)
        {
            pinNumber = p;
        }
    }
    if (pinNumber < 0) return 0;
    //find signal
    for (grp = 0; grp< GROUP_COUNT; grp++)
    {
        for (i = 0; i < MAX_PPS_SIGNALS; i++)
        {
            if (PPSinSignals[grp][i] != 0 && strcmp(PPSinSignals[grp][i], signalName) == 0)
            {
                if (PPSGroup[pinNumber] != grp) return 0;
                PPSInput(grp, fn, pin);
            }
            else
                if (PPSoutSignals[grp][i] != 0 && strcmp(PPSoutSignals[grp][i], signalName) == 0)
                {
                    if (PPSGroup[pinNumber] != grp) return 0;
                }
            
        }
    }
    return 0;
}


