
/*****************************************************************************
 * File Description:
 *
 *   This file provides configuration and basic hardware setup.
 *   For Fubarino Mini or other Pic32MX250 series
 *
 *****************************************************************************/


#include "HardwareProfile.h"
#include "FubarinoMiniPPS.h"

/* Hardware specific includes. */
#include "ConfigPerformance.h"

// PIC32MX250F128x Configuration Bit Settings

// DEVCFG3
// USERID = No Setting
#pragma config FUSBIDIO = ON            // USB USID Selection (Controlled by the USB Module)
#pragma config FVBUSONIO = ON           // USB VBUS ON Selection (Controlled by USB Module)

// DEVCFG2
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider) (= 4MHz)
#pragma config FPLLMUL = MUL_24         // PLL Multiplier (24x Multiplier) (= 96MHz)
#pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider (2x Divider)
#pragma config UPLLEN = OFF             // USB PLL Enable (Disabled and Bypassed)
#pragma config FPLLODIV = DIV_2         // System PLL Output Clock Divider (PLL Divide by 2) (= 48MHz)

// DEVCFG1
#pragma config FNOSC = PRIPLL           // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = OFF               // Internal/External Switch Over (Disabled)
#pragma config POSCMOD = HS             // Primary Oscillator Configuration (*** XT osc mode)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_2           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/2) (= 24MHz)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1024           // Watchdog Timer Postscaler (1:1024)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
#pragma config ICESEL = ICS_PGx1        // ICE/ICD Comm Channel Select (ICE EMUC2/EMUD2 pins shared with PGC1/PGD1)
#pragma config JTAGEN = OFF
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

//--------------------------------------------------------------------
//PPS Data
#ifdef PIN_DATA_MACRO
ppsAssignment_t ppsAssignment[] = PIN_DATA_MACRO;
#else
ppsAssignment_t ppsAssignment[] = {
    {0, 0}};
#endif

/*********************************************************************
 * Function:        void BoardInit( void )
 *
 ********************************************************************/

void BoardInit(void) {

    /* Configure the hardware for maximum performance. */
    vHardwareConfigurePerformance();

    /* Setup to use the external interrupt controller. */
    vHardwareUseMultiVectoredInterrupts();

    //implement PPS assignments
#ifdef PPS_ASSIGNMENT_MACRO
    PPS_ASSIGNMENT_MACRO;
#endif

    //clear all analog select
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    AD1CSSL = 0;

    //set remaining pins to output 0
    int j;
    for (j = 0; j < PIN_COUNT; j++) {
        int used = 0;
        char *_pinName = connectorPins[j];
        char *_sigName;
        if (!_pinName) break;
        //check the assignments
        int i = 0;
        while (ppsAssignment[i].pinName) {
            if (strcmp(ppsAssignment[i].pinName, _pinName) == 0) {
                used = 1;
                _sigName = ppsAssignment[i].signalName;
                break;
            }
            i++;
        }
        unsigned int ioport = GetPinIOPORT(_pinName);
        unsigned int bitMask = GetPinBitMask(_pinName);
        if (used) {
            if (strncmp(_sigName, "AN", 2) == 0)
            {
                PORTSetPinsAnalogIn(ioport, bitMask);
            }
        } else
        {
            PORTSetPinsDigitalOut(ioport, bitMask);
            PORTClearBits(ioport, bitMask);
        }
    }
#if USING_UART_1
    SetPinDigitalOut(GetSigPinName("U1TX"));
    SetPinDigitalIn(GetSigPinName("U1RX"));
#endif
#if USING_UART_2
    SetPinDigitalOut(GetSigPinName("U2TX"));
    SetPinDigitalIn(GetSigPinName("U2RX"));
#endif
#if USING_I2C_1
    SetPinDigitalIn(GetSigPinName("SDA1"));
    SetPinDigitalIn(GetSigPinName("SCL1"));    
#endif
#if USING_I2C_2
    SetPinDigitalIn(GetSigPinName("SDA2"));
    SetPinDigitalIn(GetSigPinName("SCL2"));
#endif
}

int GetPinIOPORT(char *_pinName) {
    char ioportLetter = 'Z';
    sscanf(_pinName, "RP%1c", &ioportLetter);
    switch (ioportLetter) {
        case 'A':
        case 'a':
            return IOPORT_A;
            break;
        case 'B':
        case 'b':
            return IOPORT_B;
            break;
        case 'C':
        case 'c':
            return IOPORT_C;
            break;
        default:
            return -1;
            break;
    }
}

unsigned int GetPinBitMask(char *_pinName) {
    if (_pinName && strlen(_pinName) > 3) {

        int bitNumber = _pinName[3] - '0';

        if (strlen(_pinName) > 4) {
            bitNumber = bitNumber * 10 + (_pinName[4] - '0');
        }
        return (1 << bitNumber);
    } else
        return 0;
}

char *GetSigPinName(char *_sigName) {
    char *result = 0;
    int i = 0;
    while (ppsAssignment[i].pinName) {
        if (strcmp(ppsAssignment[i].signalName, _sigName) == 0) {
            result = ppsAssignment[i].pinName;
            break;
        }
        i++;
    }
    return result;
}

char *GetPinSigName(char *_pinName) {
    char *result = 0;
    int i = 0;
    while (ppsAssignment[i].pinName) {
        if (!strcmp(ppsAssignment[i].pinName, _pinName)) {
            result = ppsAssignment[i].signalName;
            break;
        }
        i++;
    }
    return result;
}

int GetSigIOPORT(char *_sigName) {
    return GetPinIOPORT(GetSigPinName(_sigName));
}

unsigned int GetSigBitMask(char *_sigName) {
    return GetPinBitMask(GetSigPinName(_sigName));
}

void SetPinDigitalOut(char *_pinName) {
    unsigned int ioport = GetPinIOPORT(_pinName);
    unsigned int bitMask = GetPinBitMask(_pinName);

    PORTSetPinsDigitalOut(ioport, bitMask);
}

void SetPinDigitalIn(char *_pinName) {
    unsigned int ioport = GetPinIOPORT(_pinName);
    unsigned int bitMask = GetPinBitMask(_pinName);

    PORTSetPinsDigitalIn(ioport, bitMask);
}

void DigitalWrite(char *_pinName, int value) {
    unsigned int ioport = GetPinIOPORT(_pinName);
    unsigned int bitMask = GetPinBitMask(_pinName);
    if (value) {
        PORTSetBits(ioport, bitMask);
    }
    else {
        PORTClearBits(ioport, bitMask);
    }
}

char DigitalRead(char *_pinName) {
    unsigned int ioport = GetPinIOPORT(_pinName);
    unsigned int bitMask = GetPinBitMask(_pinName);

    return PORTReadBits(ioport, bitMask);
}
