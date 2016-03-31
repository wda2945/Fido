#include <xc.h>
#include <sys/asm.h>
#include "ISR_Support.h"
#include "SoftwareProfile.h"


	.set	nomips16
 	.set 	noreorder
#ifdef USING_I2C_1
 	.extern I2C_1_ISR_Handler
#endif
#ifdef USING_I2C_2
    .extern I2C_2_ISR_Handler
#endif
#ifdef USING_I2C_3
 	.extern I2C_3_ISR_Handler
#endif
#ifdef USING_I2C_4
 	.extern I2C_4_ISR_Handler
#endif
#ifdef USING_I2C_5
    .extern I2C_5_ISR_Handler
#endif
	.extern xISRStackTop

#ifdef USING_I2C_1
 	.global I2C_1_ISR_Wrapper
#endif
#ifdef USING_I2C_2
    .global I2C_2_ISR_Wrapper
#endif
#ifdef USING_I2C_3
 	.global I2C_3_ISR_Wrapper
#endif
#ifdef USING_I2C_4
 	.global I2C_4_ISR_Wrapper
#endif
#ifdef USING_I2C_5
 	.global I2C_5_ISR_Wrapper
#endif


/******************************************************************/
#ifdef USING_I2C_1
 	.set		noreorder
	.set 		noat
 	.ent		I2C_1_ISR_Wrapper

I2C_1_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		I2C_1_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end I2C_1_ISR_Wrapper
#endif
/******************************************************************/
#ifdef USING_I2C_2
 	.set		noreorder
	.set 		noat
 	.ent		I2C_2_ISR_Wrapper

I2C_2_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		I2C_2_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end I2C_2_ISR_Wrapper

#endif
/******************************************************************/
#ifdef USING_I2C_3
 	.set		noreorder
	.set 		noat
 	.ent		I2C_3_ISR_Wrapper

I2C_3_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		I2C_3_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end I2C_3_ISR_Wrapper


#endif
/******************************************************************/
#ifdef USING_I2C_4

 	.set		noreorder
	.set 		noat
 	.ent		I2C_4_ISR_Wrapper

I2C_4_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		I2C_4_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end I2C_4_ISR_Wrapper


#endif
/******************************************************************/
#ifdef USING_I2C_5

 	.set		noreorder
	.set 		noat
 	.ent		I2C_5_ISR_Wrapper

I2C_5_ISR_Wrapper:

	portSAVE_CONTEXT

	jal 		I2C_5_ISR_Handler
	nop

	portRESTORE_CONTEXT

	.end I2C_5_ISR_Wrapper

#endif
/******************************************************************/
