

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "HardwareProfile.h"

#include "SysLog/SysLog.h"

/*-----------------------------------------------------------*/
/*
 * Create the tasks then start the scheduler.
 */
void MCPtaskInit();

int main(void) {
    /* Prepare the hardware */
    BoardInit();

    MCPtaskInit();
 
    vTaskStartScheduler();
    return 0;
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void) {
    /* vApplicationMallocFailedHook() will only be called if
    configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
    function that will get called if a call to pvPortMalloc() fails.
    pvPortMalloc() is called internally by the kernel whenever a task, queue,
    timer or semaphore is created.  It is also called by various parts of the
    demo application.  If heap_1.c or heap_2.c are used, then the size of the
    heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
    FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
    to query the size of free heap space that remains (although it does not
    provide information on how the remaining heap might be fragmented). */
    taskDISABLE_INTERRUPTS();

    SysLog(SYSLOG_FAILURE, "MALLOC Failed");
    for (;;);
}

/*-----------------------------------------------------------*/

void vApplicationIdleHook(void) {
    /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
    to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
    task.  It is essential that code added to this hook function never attempts
    to block in any way (for example, call xQueueReceive() with a block time
    specified, or call vTaskDelay()).  If the application makes use of the
    vTaskDelete() API function (as this demo application does) then it is also
    important that vApplicationIdleHook() is permitted to return to its calling
    function, because it is the responsibility of the idle task to clean up
    memory allocated by the kernel to any task that has since been deleted. */
}

/*-----------------------------------------------------------*/
int reOverflow = 0;

void vApplicationStackOverflowHook(TaskHandle_t pxTask, signed char *pcTaskName) {
    (void) pcTaskName;
    (void) pxTask;

    /* Run time task stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook	function is
    called if a task stack overflow is detected.  Note the system/interrupt
    stack is not checked. */
//    taskDISABLE_INTERRUPTS();

//    if (reOverflow == 0) {
//        SysLog(SYSLOG_FAILURE, "Stack: %s", pcTaskName);
//        reOverflow = 1;
//    }
    for (;;);
}

/*-----------------------------------------------------------*/

void vApplicationTickHook(void) {
    /* This function will be called by each tick interrupt if
    configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
    added here, but the tick hook is called from an interrupt context, so
    code must not attempt to block, and only the interrupt safe FreeRTOS API
    functions can be used (those that end in FromISR()). */
}

/*-----------------------------------------------------------*/
static unsigned int _excep_code;
static unsigned int _excep_addr;

// Declared static in case exception condition would prevent
// an auto variable from being created
static enum {
	EXCEP_IRQ = 0,      // interrupt
	EXCEP_AdEL = 4,     // address error exception (load or ifetch)
	EXCEP_AdES,         // address error exception (store)
	EXCEP_IBE,          // bus error (ifetch)
	EXCEP_DBE,          // bus error (load/store)
	EXCEP_Sys,          // syscall
	EXCEP_Bp,           // breakpoint
	EXCEP_RI,           // reserved instruction
	EXCEP_CpU,          // coprocessor unusable
	EXCEP_Overflow,     // arithmetic overflow
	EXCEP_Trap,         // trap (possible divide by zero)
	EXCEP_IS1 = 16,     // implementation specfic 1
	EXCEP_CEU,          // CorExtend Unuseable
	EXCEP_C2E,           // coprocessor 2
    EXCEP_COUNT
} _excep_code;

static char *exceptionCode[] = {
    "interrupt",
	"address error exception (load or ifetch)",
	"address error exception (store)",
	"bus error (ifetch)",
	"bus error (load/store)",
	"syscall",
	"breakpoint",
	"reserved instruction",
	"coprocessor unusable",
	"arithmetic overflow",
	"trap (possible divide by zero)",
	"implementation specfic",
	"CorExtend Unuseable",
	"coprocessor 2"
};

static char exceptionMessage[80];

void _general_exception_handler(unsigned long ulCause, unsigned long ulStatus) {
    /* This overrides the definition provided by the kernel.  Other exceptions
    should be handled here. */

    asm volatile("mfc0 %0,$13" : "=r" (_excep_code));
    asm volatile("mfc0 %0,$14" : "=r" (_excep_addr));

    _excep_code = (_excep_code & 0x0000007C) >> 2;

    if (_excep_code < EXCEP_COUNT)
    _ExceptionMessage("\nGeneral Exception\n");
     if (_excep_code < EXCEP_COUNT)
        sprintf(exceptionMessage, "Gen Except @ %x : %s\n", _excep_addr, exceptionCode[_excep_code]);
     else
        sprintf(exceptionMessage, "Gen Except @ %x : 0x%02x\n", _excep_addr, _excep_code);
    _ExceptionMessage(exceptionMessage);
    
    while (1)
    {
#define COUNT1 1000000
        long i;
        for (i=0;i<COUNT1;i++);
        PORTToggleBits(USER_LED_IOPORT, USER_LED_BIT);
    }
}

/*-----------------------------------------------------------*/

void vAssertCalled(const char * pcFile, unsigned long ulLine) {
    volatile unsigned long ul = 0;

//    taskENTER_CRITICAL();
    {
        sprintf(exceptionMessage, "\nvAssert @ %s : %li\n", pcFile, ulLine);
        _ExceptionMessage(exceptionMessage);
        
        /* Set ul to a non-zero value using the debugger to step out of this
        function. */
        while (ul == 0) {
#define COUNT2 2000000
            long i;
            for (i = 0; i < COUNT2; i++);
            PORTToggleBits(USER_LED_IOPORT, USER_LED_BIT);
        }
    }
//    taskEXIT_CRITICAL();
}
