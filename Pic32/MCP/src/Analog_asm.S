#include <xc.h>
#include <sys/asm.h>
#include "ISR_Support.h"
#include "SoftwareProfile.h"


	.set	nomips16
 	.set 	noreorder

 	.extern ADC_ISR_Handler

	.extern xISRStackTop

 	.global ADC_ISR_Wrapper

/******************************************************************/

 	.set		noreorder
	.set 		noat
 	.ent		ADC_ISR_Wrapper

ADC_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		ADC_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end ADC_ISR_Wrapper

/******************************************************************/
