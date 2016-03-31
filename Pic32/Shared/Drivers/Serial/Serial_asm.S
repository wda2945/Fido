#include <xc.h>
#include <sys/asm.h>
#include "ISR_Support.h"

	.set	nomips16
 	.set 	noreorder

 	.extern UART_1A_ISR_Handler
 	.extern UART_2A_ISR_Handler
 	.extern UART_3A_ISR_Handler
 	.extern UART_1B_ISR_Handler
	.extern UART_2B_ISR_Handler
 	.extern UART_3B_ISR_Handler

	.extern xISRStackTop

 	.global UART_1A_ISR_Wrapper
 	.global UART_2A_ISR_Wrapper
 	.global UART_3A_ISR_Wrapper
 	.global UART_1B_ISR_Wrapper
 	.global UART_2B_ISR_Wrapper
 	.global UART_3B_ISR_Wrapper


/******************************************************************/
//#ifdef _UART1A
 	.set		noreorder
	.set 		noat
 	.ent		UART_1A_ISR_Wrapper

UART_1A_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		UART_1A_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end UART_1A_ISR_Wrapper

//#endif
/******************************************************************/
//#ifdef _UART3A

 	.set		noreorder
	.set 		noat
 	.ent		UART_3A_ISR_Wrapper

UART_3A_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		UART_3A_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end UART_3A_ISR_Wrapper


//#endif
/******************************************************************/
//#ifdef _UART2A

 	.set		noreorder
	.set 		noat
 	.ent		UART_2A_ISR_Wrapper

UART_2A_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		UART_2A_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end UART_2A_ISR_Wrapper

//#endif
/******************************************************************/
//#ifdef _UART1B

 	.set		noreorder
	.set 		noat
 	.ent		UART_1B_ISR_Wrapper

UART_1B_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		UART_1B_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end UART_1B_ISR_Wrapper

//#endif
/******************************************************************/
//#ifdef _UART3B

 	.set		noreorder
	.set 		noat
 	.ent		UART_3B_ISR_Wrapper

UART_3B_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		UART_3B_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end UART_3B_ISR_Wrapper

//#endif
/******************************************************************/
//#ifdef _UART2B

 	.set		noreorder
	.set 		noat
 	.ent		UART_2B_ISR_Wrapper

UART_2B_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		UART_2B_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end UART_2B_ISR_Wrapper

//#endif