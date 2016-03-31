
/*****************************************************************************
 * File Description:
 *
 *   This file provides configuration and basic hardware setup.
 *
 *****************************************************************************/

#include <xc.h>

#include "HardwareProfile.h"

#define _SUPPRESS_PLIB_WARNING
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING

#include "plib.h"

/* Hardware specific includes. */
#include "Platforms/ConfigPerformance.h"

// PIC32MX795F512H Configuration Bit Settings

// DEVCFG3
// USERID = No Setting
#pragma config FSRSSEL = PRIORITY_7     // SRS Select (SRS Priority 7)
#pragma config FMIIEN = OFF             // Ethernet RMII/MII Enable (RMII Enabled)
#pragma config FETHIO = ON              // Ethernet I/O Pin Select (Default Ethernet I/O)
#pragma config FCANIO = OFF             // CAN I/O Pin Select (Alternate CAN I/O)
#pragma config FUSBIDIO = ON            // USB USID Selection (Controlled by the USB Module)
#pragma config FVBUSONIO = ON           // USB VBUS ON Selection (Controlled by USB Module)

// DEVCFG2
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (20x Multiplier)
#pragma config UPLLIDIV = DIV_12        // USB PLL Input Divider (12x Divider)
#pragma config UPLLEN = OFF             // USB PLL Enable (Disabled and Bypassed)
#pragma config FPLLODIV = DIV_1         // System PLL Output Clock Divider (PLL Divide by 1)

// DEVCFG1
#pragma config FNOSC = PRIPLL           // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = OFF               // Internal/External Switch Over (Disabled)
#pragma config POSCMOD = HS             // Primary Oscillator Configuration (*** XT osc mode)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_2           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/2)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1024           // Watchdog Timer Postscaler (1:1024)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
#pragma config ICESEL = ICS_PGx2        // ICE/ICD Comm Channel Select (ICE EMUC2/EMUD2 pins shared with PGC2/PGD2)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

/*********************************************************************
 * Function:        void BoardInit( void )
 *
 ********************************************************************/

void BoardInit(void) {

    /* Configure the hardware for maximum performance. */
    vHardwareConfigurePerformance();

    /* Setup to use the external interrupt controller. */
    vHardwareUseMultiVectoredInterrupts();


//Set up the serial port input pins
    //UART1 Rx=RD2, Tx=RD3  U1A
#if (USING_UART_1A || USING_UART_1A_RX)
   PORTSetPinsDigitalIn(PIN_8_IOPORT, PIN_8_BIT);
#define USING_PIN_8 1
#endif
    //UART2 Rx=RF4, Tx=RF5  U3A
#if (USING_UART_3A || USING_UART_3A_RX)
   PORTSetPinsDigitalIn(PIN_28_IOPORT, PIN_28_BIT);
#define USING_PIN_28 1
#endif
    //UART3 Rx=RG7, Tx=RG8  U2A
#if (USING_UART_2A || USING_UART_2A_RX)
   PORTSetPinsDigitalIn(PIN_25_IOPORT, PIN_25_BIT);
#define USING_PIN_25 1
#endif
    //UART4 Rx=RD9, Tx=RD1  U1B
#if (USING_UART_1B || USING_UART_1B_RX)
   PORTSetPinsDigitalIn(PIN_1_IOPORT, PIN_1_BIT);
#define USING_PIN_1 1
#endif
    //UART5 Rx=RB8, Tx=RB14 U3B
#if (USING_UART_3B || USING_UART_3B_RX)
   PORTSetPinsDigitalIn(PIN_A7_IOPORT, PIN_A7_BIT);
#define USING_PIN_A7 1
#endif
    //UART6 Rx=RG9, Tx=RG6  U2B
#if (USING_UART_2B || USING_UART_2B_RX)
   PORTSetPinsDigitalIn(PIN_27_IOPORT, PIN_27_BIT);
#define USING_PIN_27 1
#endif

//Set up the serial port output pins
//UART1 Rx=RD2, Tx=RD3  U1A
#if (USING_UART_1A || USING_UART_1A_TX)
   PORTSetPinsDigitalOut(PIN_9_IOPORT, PIN_9_BIT);
   PORTSetBits(PIN_9_IOPORT, PIN_9_BIT);
#define USING_PIN_9 1
#endif
    //UART2 Rx=RF4, Tx=RF5  U3A
#if (USING_UART_3A || USING_UART_3A_TX)
   PORTSetPinsDigitalOut(PIN_29_IOPORT, PIN_29_BIT);
   PORTSetBits(PIN_29_IOPORT, PIN_29_BIT);
#define USING_PIN_29 1
#endif
    //UART3 Rx=RG7, Tx=RG8  U2A
#if (USING_UART_2A || USING_UART_2A_TX)
   PORTSetPinsDigitalOut(PIN_26_IOPORT, PIN_26_BIT);
   PORTSetBits(PIN_26_IOPORT, PIN_26_BIT);
#define USING_PIN_26 1
#endif
    //UART4 Rx=RD9, Tx=RD1  U1B
#if (USING_UART_1B || USING_UART_1B_TX)
   PORTSetPinsDigitalOut(PIN_7_IOPORT, PIN_7_BIT);
   PORTSetBits(PIN_7_IOPORT, PIN_7_BIT);
#define USING_PIN_7 1
#endif
    //UART5 Rx=RB8, Tx=RB14 U3B
#if (USING_UART_3B || USING_UART_3B_TX)
   PORTSetPinsDigitalOut(PIN_A1_IOPORT, PIN_A1_BIT);
   PORTSetBits(PIN_A1_IOPORT, PIN_A1_BIT);
#define USING_PIN_A1 1
#endif
    //UART6 Rx=RG9, Tx=RG6  U2B
#if (USING_UART_2B || USING_UART_2B_TX)
   PORTSetPinsDigitalOut(PIN_24_IOPORT, PIN_24_BIT);
   PORTSetBits(PIN_24_IOPORT, PIN_24_BIT);
#define USING_PIN_24 1
#endif

   //set up the SPI pins
#if USING_SPI_2A
   PORTSetPinsDigitalIn(PIN_28_IOPORT, PIN_25_BIT);     //SPI2 MISO
#define USING_PIN_24 1
#define USING_PIN_25 1
#define USING_PIN_26 1
#define USING_PIN_27 1
#endif
#if USING_SPI_1A
   PORTSetPinsDigitalIn(PIN_8_IOPORT, PIN_8_BIT);       //SPI3 MISO
#define USING_PIN_1 1
#define USING_PIN_7 1
#define USING_PIN_8 1
#define USING_PIN_9 1
 #endif
#if USING_SPI_3A
    PORTSetPinsDigitalIn(PIN_28_IOPORT, PIN_28_BIT);     //SPI4 MISO
#define USING_PIN_A7 1
#define USING_PIN_A1 1
#define USING_PIN_28 1
#define USING_PIN_29 1
#endif

#if USING_I2C_1
#define USING_PIN_1 1
#define USING_PIN_2 1
#endif
#if USING_I2C_1A
#define USING_PIN_8 1
#define USING_PIN_9 1
#endif
#if USING_I2C_2A
#define USING_PIN_25 1
#define USING_PIN_26 1
#endif
#if USING_I2C_3A
#define USING_PIN_28 1
#define USING_PIN_29 1
#endif

#include "CheckPinUsage.h"

DDPCON = 0;

#define SET_PIN_ZERO(p) PORTSetPinsDigitalOut(p##_IOPORT, p##_BIT);PORTClearBits(p##_IOPORT, p##_BIT);


#if !USING_PIN_0
SET_PIN_ZERO(PIN_0)
#endif
#if !USING_PIN_1
SET_PIN_ZERO(PIN_1)
#endif
#if !USING_PIN_2
SET_PIN_ZERO(PIN_2)
#endif
#if !USING_PIN_3
SET_PIN_ZERO(PIN_3)
#endif
#if !USING_PIN_4
SET_PIN_ZERO(PIN_4)
#endif
#if !USING_PIN_5
SET_PIN_ZERO(PIN_5)
#endif
#if !USING_PIN_6
SET_PIN_ZERO(PIN_6)
#endif
#if !USING_PIN_7
SET_PIN_ZERO(PIN_7)
#endif
#if !USING_PIN_8
SET_PIN_ZERO(PIN_8)
#endif
#if !USING_PIN_9
SET_PIN_ZERO(PIN_9)
#endif
#if !USING_PIN_10
SET_PIN_ZERO(PIN_10)
#endif
#if !USING_PIN_11
SET_PIN_ZERO(PIN_11)
#endif
#if !USING_PIN_12
SET_PIN_ZERO(PIN_12)
#endif
#if !USING_PIN_13
SET_PIN_ZERO(PIN_13)
#endif
#if !USING_PIN_14
SET_PIN_ZERO(PIN_14)
#endif
#if !USING_PIN_15
SET_PIN_ZERO(PIN_15)
#endif
#if !USING_PIN_16
SET_PIN_ZERO(PIN_16)
#endif
#if !USING_PIN_17
SET_PIN_ZERO(PIN_17)
#endif
#if !USING_PIN_18
SET_PIN_ZERO(PIN_18)
#endif
#if !USING_PIN_19
SET_PIN_ZERO(PIN_19)
#endif
#if !USING_PIN_20
SET_PIN_ZERO(PIN_20)
#endif
#if !USING_PIN_21
SET_PIN_ZERO(PIN_21)
#endif
#if !USING_PIN_22
SET_PIN_ZERO(PIN_22)
#endif
#if !USING_PIN_23
SET_PIN_ZERO(PIN_23)
#endif

#if !USING_PIN_24
SET_PIN_ZERO(PIN_24)
#endif
#if !USING_PIN_25
SET_PIN_ZERO(PIN_25)
#endif
#if !USING_PIN_26
SET_PIN_ZERO(PIN_26)
#endif
#if !USING_PIN_27
SET_PIN_ZERO(PIN_27)
#endif
#if !USING_PIN_A14
SET_PIN_ZERO(PIN_A14)
#endif
#if !USING_PIN_A13
SET_PIN_ZERO(PIN_A13)
#endif
#if !USING_PIN_A12
SET_PIN_ZERO(PIN_A12)
#endif
#if !USING_PIN_A11
SET_PIN_ZERO(PIN_A11)
#endif
#if !USING_PIN_A10
SET_PIN_ZERO(PIN_A10)
#endif
#if !USING_PIN_A9
SET_PIN_ZERO(PIN_A9)
#endif
#if !USING_PIN_A8
SET_PIN_ZERO(PIN_A8)
#endif
#if !USING_PIN_A7
SET_PIN_ZERO(PIN_A7)
#endif
#if !USING_PIN_A6
SET_PIN_ZERO(PIN_A6)
#endif
#if !USING_PIN_A5
SET_PIN_ZERO(PIN_A5)
#endif
#if !USING_PIN_A4
SET_PIN_ZERO(PIN_A4)
#endif
#if !USING_PIN_A3
SET_PIN_ZERO(PIN_A3)
#endif
#if !USING_PIN_A2
SET_PIN_ZERO(PIN_A2)
#endif
#if !USING_PIN_A1
SET_PIN_ZERO(PIN_A1)
#endif
#if !USING_PIN_A0
SET_PIN_ZERO(PIN_A0)
#endif
#if !USING_PIN_28
SET_PIN_ZERO(PIN_28)
#endif
#if !USING_PIN_29
SET_PIN_ZERO(PIN_29)
#endif

}
