/* 
 * File:   Fubarino.h
 * Author: martin
 *
 * Created on December 15, 2013, 10:18 PM
 */

#ifndef FUBARINO_H
#define	FUBARINO_H

#define PIN_0_IOPORT IOPORT_D	//Pin 0 RD8 RTCC/IC1/INT1
#define PIN_0_BIT BIT_8
#define PIN_1_IOPORT IOPORT_D	//Pin 1 RD9 SS1A/U1BRX/U1ACTS/SDA1/IC2/INT2
#define PIN_1_BIT BIT_9
#define PIN_2_IOPORT IOPORT_D	//Pin 2 RD10 SCL1/IC3/PMCS2/PMA15/INT3
#define PIN_2_BIT BIT_10
#define PIN_3_IOPORT IOPORT_D	//Pin 3 RD11 IC4/PMCS1/PMA14/INT4
#define PIN_3_BIT BIT_11
#define PIN_4_IOPORT IOPORT_D	//Pin 4 RD0 OC1/INT0/RD0
#define PIN_4_BIT BIT_0
#define PIN_5_IOPORT IOPORT_C	//Pin 5 RC13 SOSCI/CN1
#define PIN_5_BIT BIT_13
#define PIN_6_IOPORT IOPORT_C	//Pin 6 RC14 SOSCO/T1CK/CN0
#define PIN_6_BIT BIT_14
#define PIN_7_IOPORT IOPORT_D	//Pin 7 RD1 SCK1A/U1BTX/U1ARTS/OC2
#define PIN_7_BIT BIT_1
#define PIN_8_IOPORT IOPORT_D	//Pin 8 RD2 SDA1A/SDI1A/U1ARX/OC3
#define PIN_8_BIT BIT_2
#define PIN_9_IOPORT IOPORT_D	//Pin 9 RD3 SCL1A/SDO1A/U1ATX/OC4
#define PIN_9_BIT BIT_3
#define PIN_10_IOPORT IOPORT_D	//Pin 10 RD4 OC5/IC5/PMWR/CN13
#define PIN_10_BIT BIT_4
#define PIN_11_IOPORT IOPORT_D	//Pin 11 RD5 PMRD/CN14
#define PIN_11_BIT BIT_5
#define PIN_12_IOPORT IOPORT_D	//Pin 12 RD6 CN15
#define PIN_12_BIT BIT_6
#define PIN_13_IOPORT IOPORT_D	//Pin 13 RD7 CN16
#define PIN_13_BIT BIT_7
#define PIN_14_IOPORT IOPORT_F	//Pin 14 RF0
#define PIN_14_BIT BIT_0
#define PIN_15_IOPORT IOPORT_F	//Pin 15 RF1
#define PIN_15_BIT BIT_1
#define PIN_16_IOPORT IOPORT_E	//Pin 16 RE0 PMD0
#define PIN_16_BIT BIT_0
#define PIN_17_IOPORT IOPORT_E	//Pin 17 RE1 PMD1
#define PIN_17_BIT BIT_1
#define PIN_18_IOPORT IOPORT_E	//Pin 18 RE2 PMD2
#define PIN_18_BIT BIT_2
#define PIN_19_IOPORT IOPORT_E	//Pin 19 RE3 PMD3
#define PIN_19_BIT BIT_3
#define PIN_20_IOPORT IOPORT_E	//Pin 20 RE4 PMD4
#define PIN_20_BIT BIT_4
#define PIN_21_IOPORT IOPORT_E	//Pin 21 RE5 PMD5
#define PIN_21_BIT BIT_5
#define PIN_22_IOPORT IOPORT_E	//Pin 22 RE6 PMD6
#define PIN_22_BIT BIT_6
#define PIN_23_IOPORT IOPORT_E	//Pin 23 RE7 PMD7
#define PIN_23_BIT BIT_7
#define PIN_24_IOPORT IOPORT_G	//Pin 24  RG6 SCK2A/U2BTX/U2ARTS/PMA5/CN8
#define PIN_24_BIT BIT_6
#define PIN_25_IOPORT IOPORT_G	//Pin 25  RG7 SDA2A/SDI2A/U2ARX/PMA4/CN9
#define PIN_25_BIT BIT_7
#define PIN_26_IOPORT IOPORT_G	//Pin 26  RG8 SCL2A/SDO2A/U2ATX/PMA3/CN10
#define PIN_26_BIT BIT_8
#define PIN_27_IOPORT IOPORT_G	//Pin 27  RG9 SS2A/U2BRX/U2ACTS/PMA2/CN11
#define PIN_27_BIT BIT_9
#define PIN_A14_IOPORT IOPORT_B	//Pin 30  RB4 AN4/C1IN-/CN6
#define PIN_A14_BIT BIT_4
#define PIN_A13_IOPORT IOPORT_B	//Pin 31  RB3 AN3/C2IN+/CN5
#define PIN_A13_BIT BIT_3
#define PIN_A12_IOPORT IOPORT_B	//Pin 32  RB2 AN2/C2IN-/CN4
#define PIN_A12_BIT BIT_2
#define PIN_A11_IOPORT IOPORT_B	//Pin 33  RB1 PGEC1/AN1/VREF-/CVREF-/CN3
#define PIN_A11_BIT BIT_1
#define PIN_A10_IOPORT IOPORT_B	//Pin 34  RB0 PGED1/AN0/VREG+/CVREF+/PMA6/CN2
#define PIN_A10_BIT BIT_0
#define PIN_A9_IOPORT IOPORT_B	// Pin 35  RB7 PGED2/AN7
#define PIN_A9_BIT BIT_7
#define PIN_A8_IOPORT IOPORT_B	// Pin 36  RB6 PGEC2/AN6/OCFA
#define PIN_A8_BIT BIT_6
#define PIN_A7_IOPORT IOPORT_B	// Pin 37  RB8 AN8/SS3A/U3BRX/USACTS/C1OUT
#define PIN_A7_BIT BIT_8
#define PIN_A6_IOPORT IOPORT_B	// Pin 38  RB9 AN9/C2OUT/PMA7
#define PIN_A6_BIT BIT_9
#define PIN_A5_IOPORT IOPORT_B	// Pin 39  RB10 TMS/AN10/CVREFOUT/PMA13
#define PIN_A5_BIT BIT_10
#define PIN_A4_IOPORT IOPORT_B	// Pin 40  RB11 TDO/AN11/PMA12/RB11
#define PIN_A4_BIT BIT_11
#define PIN_A3_IOPORT IOPORT_B	// Pin 41  RB12 TCK/AN12/PMA11
#define PIN_A3_BIT BIT_12
#define PIN_A2_IOPORT IOPORT_B	// Pin 42  RB13 TDI/AN13/PMA10
#define PIN_A2_BIT BIT_13
#define PIN_A1_IOPORT IOPORT_B	// Pin 43  RB14 AN14/SCK3A/U3BTX/U3RTS/PMALH/PMA1
#define PIN_A1_BIT BIT_14
#define PIN_A0_IOPORT IOPORT_B	// Pin 44  RB15 AN15/OCFB/PMALL/PMA0/CN12
#define PIN_A0_BIT BIT_15
#define PIN_28_IOPORT IOPORT_F	// Pin 28  RF4 SDA3A/SDI3A/U3ARX/PMA9/CN17
#define PIN_28_BIT BIT_4
#define PIN_29_IOPORT IOPORT_F	// Pin 29  RF5 SCL3A/SDO3A/U3ATX/PMA8/CN18
#define PIN_29_BIT BIT_5

//UART Regs
typedef struct
{
	volatile unsigned int	reg;
	volatile unsigned int	clr;
	volatile unsigned int	set;
	volatile unsigned int	inv;
}P32_REG_SET;

typedef struct
{
	volatile P32_REG_SET	mode;
	volatile P32_REG_SET	sta;
	volatile P32_REG_SET    tx;
	volatile P32_REG_SET	rx;
	volatile P32_REG_SET	brg;
}UART_REG_SET;

extern UART_REG_SET * const uartRegs[];

#define UART_BASE_ADDRESSES {_UART1_BASE_ADDRESS,_UART2_BASE_ADDRESS,_UART2_BASE_ADDRESS,\
    _UART4_BASE_ADDRESS,_UART5_BASE_ADDRESS,_UART6_BASE_ADDRESS}

#define UART_IRQS {_UART1_ERR_IRQ,_UART2_ERR_IRQ,_UART3_ERR_IRQ,_UART4_ERR_IRQ,\
    _UART5_ERR_IRQ,_UART6_ERR_IRQ}

#define UART_VECTOR_NUMBERS {_UART_1_VECTOR,_UART_2_VECTOR,_UART_3_VECTOR,\
    _UART_4_VECTOR,_UART_5_VECTOR,_UART_6_VECTOR}



#endif	/* FUBARINO_H */

