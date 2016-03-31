#include <xc.h>
#include <sys/asm.h>
#include "ISR_Support.h"


	.set	nomips16
 	.set 	noreorder

 	.extern Encoder_1_ISR_Handler
 	.extern Encoder_2_ISR_Handler
 	.extern Encoder_3_ISR_Handler
 	.extern Encoder_4_ISR_Handler

	.extern xISRStackTop

 	.global Encoder_1_ISR_Wrapper
 	.global Encoder_2_ISR_Wrapper
 	.global Encoder_3_ISR_Wrapper
 	.global Encoder_4_ISR_Wrapper


/******************************************************************/

 	.set		noreorder
	.set 		noat
 	.ent		Encoder_1_ISR_Wrapper

Encoder_1_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		Encoder_1_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end Encoder_1_ISR_Wrapper


/******************************************************************/

 	.set		noreorder
	.set 		noat
 	.ent		Encoder_2_ISR_Wrapper

Encoder_2_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		Encoder_2_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end Encoder_2_ISR_Wrapper


/******************************************************************/

 	.set		noreorder
	.set 		noat
 	.ent		Encoder_3_ISR_Wrapper

Encoder_3_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		Encoder_3_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end Encoder_3_ISR_Wrapper


/******************************************************************/

 	.set		noreorder
	.set 		noat
 	.ent		Encoder_4_ISR_Wrapper

Encoder_4_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		Encoder_4_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end Encoder_4_ISR_Wrapper



