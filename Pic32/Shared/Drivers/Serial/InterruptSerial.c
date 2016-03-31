/*
 * File:   InterruptSerial.c
 * Author: martin
 *
 * Created on December 6, 2013, 10:29 PM
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"

#include "Drivers/Serial/Serial.h"
#include "SysLog/SysLog.h"

#define _SUPPRESS_PLIB_WARNING
#include <plib.h>

// Define constants and variables for buffering incoming serial data.  We're
// using a ring buffer, in which buffer_head is the index of the
// location to which to write the next incoming character and buffer_tail
// is the index of the location from which to read.
// The algorithms used to operate on the head and tail assume that the
// size is a power of 2. (e.g. 32, 64, 128, etc)

typedef struct {
    UART_DATA *buffer;
    int head;
    int tail;
    int bufferSize;
} RingBuffer_t;

RingBuffer_t rx_buffer[UART_NUMBER_OF_MODULES];
RingBuffer_t tx_buffer[UART_NUMBER_OF_MODULES];

//semaphores used by the TX and RX interrupts
SemaphoreHandle_t rxSemaphore[UART_NUMBER_OF_MODULES];
SemaphoreHandle_t txSemaphore[UART_NUMBER_OF_MODULES];
#define SEMPHR_WAIT     portMAX_DELAY     //how long to wait on a semaphore before re-checking (ms Ticks)

bool Serial_begin(UART_MODULE uart, unsigned long baudRate,
        unsigned int lineControl, int _rxBufferSize, int _txBufferSize) {

    //disable all interrupts while configuring
    INTEnable(INT_U1E + uart, INT_DISABLED);
    INTEnable(INT_U1RX + uart, INT_DISABLED);
    INTEnable(INT_U1TX + uart, INT_DISABLED);

    //common configuration
    UARTConfigure(uart, UART_ENABLE_PINS_TX_RX_ONLY);       //no DTR/CTS
    UARTSetLineControl(uart, lineControl);                  //eg 8/N/1
    UARTSetDataRate(uart, GetPeripheralClock(), baudRate);
    UARTSetFifoMode(uart, UART_INTERRUPT_ON_TX_BUFFER_EMPTY | UART_INTERRUPT_ON_RX_NOT_EMPTY);

    //Initialize the receive buffer.
    rx_buffer[uart].bufferSize = _rxBufferSize;
    if (_rxBufferSize) {
        rx_buffer[uart].buffer = (UART_DATA *) pvPortMalloc(_rxBufferSize * sizeof (UART_DATA));

        if (!rx_buffer[uart].buffer) return false;

        UARTEnable(uart, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_TX | UART_RX));
    } else {
        //transmit only
        UARTEnable(uart, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_TX));
    }
    rx_buffer[uart].head = rx_buffer[uart].tail = 0;

    //create the receive semaphore
    rxSemaphore[uart] = xSemaphoreCreateBinary();
    if (rxSemaphore[uart] == NULL) {
        LogError("rxSem%i", uart + 1);
        return false;
    }

    // Initialize the transmit buffer.
    tx_buffer[uart].bufferSize = _txBufferSize;
    tx_buffer[uart].buffer = (UART_DATA *) pvPortMalloc(_txBufferSize * sizeof (UART_DATA));

    if (!tx_buffer[uart].buffer) return false;

    tx_buffer[uart].head = tx_buffer[uart].tail = 0;

    //create the transmit semaphore
    txSemaphore[uart] = xSemaphoreCreateBinary();
    if (txSemaphore[uart] == NULL) {
        LogError("txSem%i", uart + 1);
        return false;
    }

    /* Set the interrupt privilege level and sub-privilege level
     */
    INTSetVectorPriority(INT_UART_1_VECTOR + uart, UART_INT_PRIORITY);
    INTSetVectorSubPriority(INT_UART_1_VECTOR + uart, UART_INT_SUB_PRIORITY);

    // Clear the interrupt flag, and set the interrupt enables for the
    // interrupts used by this UART.
    INTClearFlag(INT_U1 + uart);
    INTClearFlag(INT_U1E + uart);

    INTEnable(INT_U1E + uart, INT_ENABLED); //error interrupt

    //receive interrupt
    if (_rxBufferSize) {
        //clear out the FIFO to prevent spurious interrupts
        while (UARTReceivedDataIsAvailable(uart)) {
            UARTGetData(uart);
        }
        INTClearFlag(INT_U1RX + uart);
        INTEnable(INT_U1RX + uart, INT_ENABLED);
    } else {
        //not using Rx
        INTEnable(INT_U1RX + uart, INT_DISABLED);
    }

    //transmit interrupt disabled until we have something to send
    INTClearFlag(INT_U1TX + uart);
    INTEnable(INT_U1TX + uart, INT_DISABLED);

    return true;
}

/* ------------------------------------------------------------ */

/***	Serial_end
 **
 **	Parameters:
 **		none
 **
 **	Return Value:
 **		none
 **
 **	Errors:
 **		none
 **
 **	Description:
 **		Disable the UART and UART interrupts.
 */

void Serial_end(UART_MODULE uart) {

    INTEnable(INT_U1E + uart, INT_DISABLED);
    INTEnable(INT_U1RX + uart, INT_DISABLED);
    INTEnable(INT_U1TX + uart, INT_DISABLED);

    // Disable the UART so that the pins can be used as general purpose I/O
    UARTEnable(uart, UART_DISABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
}

/* ------------------------------------------------------------ */

/***	HardwareSerial.available
 **
 **	Parameters:
 **		none
 **
 **	Return Value:
 **		Returns the number of characters available in the receive buffer
 **
 **	Errors:
 **		none
 **
 **	Description:
 **		Return the number of characters currently available in the
 **		receive buffer.
 */

int Serial_available(UART_MODULE uart) {
    return (rx_buffer[uart].bufferSize + rx_buffer[uart].head - rx_buffer[uart].tail) % rx_buffer[uart].bufferSize;
}

/* ------------------------------------------------------------ */

/***	Serial_peek
 **
 **	Parameters:
 **		none
 **
 **	Return Value:
 **		returns the next character from the receive buffer
 **
 **	Errors:
 **		returns -1 if no characters in buffer
 **
 **	Description:
 **		This returns the next character in the receive buffer without
 **		removing it from the buffer, or -1 if no characters are in the buffer.
 */

UART_DATA Serial_peek(UART_MODULE uart) {
    UART_DATA dat;
    if (rx_buffer[uart].head == rx_buffer[uart].tail) {
        dat.__data = -1;
    } else {
        dat = rx_buffer[uart].buffer[rx_buffer[uart].tail];
    }
    return dat;
}

/* ------------------------------------------------------------ */

/***	Serial_read
 **
 **	Parameters:
 **		none
 **
 **	Return Value:
 **		next character from the receive buffer
 **
 **	Description:
 **		Return the next character from the receive buffer and remove
 **		it from the buffer.
 */

UART_DATA Serial_read(UART_MODULE uart) {
    UART_DATA dat;
    // if the head isn't ahead of the tail, we don't have any characters
    while (rx_buffer[uart].head == rx_buffer[uart].tail) {
        xSemaphoreTake(rxSemaphore[uart], SEMPHR_WAIT);
    }
    dat = rx_buffer[uart].buffer[rx_buffer[uart].tail];
    rx_buffer[uart].tail = (rx_buffer[uart].tail + 1) % rx_buffer[uart].bufferSize;
    return dat;

}

UART_DATA Serial_read_wait(UART_MODULE uart, int ticksToWait) {
    UART_DATA dat;
    // if the head isn't ahead of the tail, we don't have any characters
    if (rx_buffer[uart].head == rx_buffer[uart].tail) {
        xSemaphoreTake(rxSemaphore[uart], ticksToWait);
    }
    if (rx_buffer[uart].head != rx_buffer[uart].tail) {
        dat = rx_buffer[uart].buffer[rx_buffer[uart].tail];
        rx_buffer[uart].tail = (rx_buffer[uart].tail + 1) % rx_buffer[uart].bufferSize;
    } else {
        dat.__data = 0;
    }
    return dat;
}
/* ------------------------------------------------------------ */

/***	Serial_watch_for_address
 **
 **	Parameters:
 **		9-bit address
 **
 **	Return Value:
 **		none
 **
 **	Errors:
 **		returns -1 if no characters in buffer
 **
 **	Description:
 **		Sets the UART to wait for the address before accepting another character.
 */
void Serial_watch_for_address(UART_MODULE uart, unsigned int address) {
    UARTSetAddress(uart, address, TRUE);
}

/* ------------------------------------------------------------ */

/***	Serial_flush
 **
 **	Parameters:
 **		none
 **
 **	Return Value:
 **		none
 **
 **	Errors:
 **		none
 **
 **	Description:
 **		Empty the send buffer by waiting for the
 **		fifo to empty and the transmitter to become idle
 */
void Serial_flush(UART_MODULE uart) {
    //wait for the buffer to be empty and transmission complete
    while ((tx_buffer[uart].head != tx_buffer[uart].tail) || !UARTTransmissionHasCompleted(uart))
        xSemaphoreTake(txSemaphore[uart], 10);
}

/* ------------------------------------------------------------ */

/***	Serial_purge
 **
 **	Parameters:
 **		none
 **
 **	Return Value:
 **		none
 **
 **	Errors:
 **		none
 **
 **	Description:
 **		Empty the receive buffer by discarding any characters in
 **		the buffer.
 */

void Serial_purge(UART_MODULE uart) {
    // don't reverse this or there may be problems if the RX interrupt
    // occurs after reading the value of rx_buffer_head but before writing
    // the value to rx_buffer_tail; the previous value of rx_buffer_head
    // may be written to rx_buffer_tail, making it appear as if the buffer
    // were full, not empty.
    rx_buffer[uart].head = rx_buffer[uart].tail;
}

/* ------------------------------------------------------------ */

/***	Serial_write
 **
 **	Parameters:
 **		theChar		- the character to transmit
 **
 **	Return Value:
 **		none
 **
 **	Errors:
 **		none
 **
 **	Description:
 **		Wait until the transmitter is idle, and then transmit the
 **		specified character.
 */
void Serial_write(UART_MODULE uart, uint8_t theChar) {
    UART_DATA dat;
    dat.data8bit = theChar;
    Serial_writeData(uart, dat);
}

void Serial_writeData(UART_MODULE uart, UART_DATA dat) { //write <=9 bits

    int bufIndex = (tx_buffer[uart].head + 1) % tx_buffer[uart].bufferSize;
    /* If we should be storing the received character into the location
     ** just before the tail (meaning that the head would advance to the
     ** current location of the tail), we're about to overflow the buffer
     ** and so we don't write the character or advance the head. We wait.
     */
    while (bufIndex == tx_buffer[uart].tail) {
        xSemaphoreTake(txSemaphore[uart], SEMPHR_WAIT);
        bufIndex = (tx_buffer[uart].head + 1) % tx_buffer[uart].bufferSize;
    }
    tx_buffer[uart].buffer[tx_buffer[uart].head] = dat;
    tx_buffer[uart].head = bufIndex;
    //turn on the interrupt to start sending
    INTEnable(INT_U1TX + uart, INT_ENABLED);
}

//Write n bytes

void Serial_write_bytes(UART_MODULE uart, const unsigned char *buffer, unsigned char len) {
    int size = len;
    while (size) {
        Serial_write(uart, *buffer);
        buffer++;
        size--;
    }
}
//write a null-terminated string

void Serial_write_string(UART_MODULE uart, const unsigned char *str) {
    Serial_write_bytes(uart, str, strlen(str));
}

//************************************************
//interrupt ISRs
//Assembly wrappers
#if USING_UART_1A || USING_UART_1A_RX || USING_UART_1A_TX
void __attribute__((interrupt(UART_IPL), vector(_UART_1A_VECTOR))) UART_1A_ISR_Wrapper(void);
#endif
#if USING_UART_3A || USING_UART_3A_RX || USING_UART_3A_TX
void __attribute__((interrupt(UART_IPL), vector(_UART_3A_VECTOR))) UART_3A_ISR_Wrapper(void);
#endif
#if USING_UART_2A || USING_UART_2A_RX || USING_UART_2A_TX
void __attribute__((interrupt(UART_IPL), vector(_UART_2A_VECTOR))) UART_2A_ISR_Wrapper(void);
#endif
#if USING_UART_1B || USING_UART_1B_RX || USING_UART_1B_TX
void __attribute__((interrupt(UART_IPL), vector(_UART_1B_VECTOR))) UART_1B_ISR_Wrapper(void);
#endif
#if USING_UART_3B || USING_UART_3B_RX || USING_UART_3B_TX
void __attribute__((interrupt(UART_IPL), vector(_UART_3B_VECTOR))) UART_3B_ISR_Wrapper(void);
#endif
#if USING_UART_2B || USING_UART_2B_RX || USING_UART_2B_TX
void __attribute__((interrupt(UART_IPL), vector(_UART_2B_VECTOR))) UART_2B_ISR_Wrapper(void);
#endif


//common rx handler

void UART_RX_Handler(UART_MODULE uart) {
    while (UARTReceivedDataIsAvailable(uart)) {
        UART_DATA dat = UARTGetData(uart);
        int bufIndex = (rx_buffer[uart].head + 1) % rx_buffer[uart].bufferSize;

        /* If we should be storing the received character into the location
         ** just before the tail (meaning that the head would advance to the
         ** current location of the tail), we're about to overflow the buffer
         ** and so we don't write the character or advance the head.
         */
        if (bufIndex != rx_buffer[uart].tail) {
            rx_buffer[uart].buffer[rx_buffer[uart].head] = dat;
            rx_buffer[uart].head = bufIndex;
        } else {
            //TODO: deal with overrun condition
        }
    }
}

//common tx handler

void UART_TX_Handler(UART_MODULE uart) {
    UART_DATA dat;
    while (UARTTransmitterIsReady(uart)) {
        if (tx_buffer[uart].head != tx_buffer[uart].tail) {
            dat = tx_buffer[uart].buffer[tx_buffer[uart].tail];
            tx_buffer[uart].tail = (tx_buffer[uart].tail + 1) % tx_buffer[uart].bufferSize;
            UARTSendData(uart, dat);
        } else {
            //no more data - turn off the interrupt
            INTEnable(INT_U1TX + uart, INT_DISABLED);
            return;
        }
    }
}

//ISRs for each UART
#ifdef _UART1 // or -UART1A
void UART_1A_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    if (INTGetFlag(INT_U1RX)) {
        UART_RX_Handler(UART1); //handle an rx interrupt
        INTClearFlag(INT_U1RX);
        xSemaphoreGiveFromISR(rxSemaphore[UART1], &higherPriorityTaskWoken);
    }
    if (INTGetFlag(INT_U1E)) {
        uartReg[UART1]->sta.clr = UART_PARITY_ERROR | UART_FRAMING_ERROR | UART_OVERRUN_ERROR;
        INTClearFlag(INT_U1E);
    }
    if (INTGetFlag(INT_U1TX)) {
        UART_TX_Handler(UART1); //handle a tx interrupt
        xSemaphoreGiveFromISR(txSemaphore[UART1], &higherPriorityTaskWoken);
        INTClearFlag(INT_U1TX);
    }
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}
#endif
#ifdef _UART2 //OR _UART3A
void UART_3A_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    if (INTGetFlag(INT_U2RX)) {
        UART_RX_Handler(UART2);
        INTClearFlag(INT_U2RX);
        xSemaphoreGiveFromISR(rxSemaphore[UART2], &higherPriorityTaskWoken);
    }
    if (INTGetFlag(INT_U2E)) {
        uartReg[UART2]->sta.clr = UART_PARITY_ERROR | UART_FRAMING_ERROR | UART_OVERRUN_ERROR;
        INTClearFlag(INT_U2E);
    }
    if (INTGetFlag(INT_U2TX)) {
        UART_TX_Handler(UART2);
        xSemaphoreGiveFromISR(txSemaphore[UART2], &higherPriorityTaskWoken);
        INTClearFlag(INT_U2TX);
    }
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}
#endif
#ifdef _UART3 //OR _UART2A
void UART_2A_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    if (INTGetFlag(INT_U3RX)) {
        UART_RX_Handler(UART3);
        INTClearFlag(INT_U3RX);
        xSemaphoreGiveFromISR(rxSemaphore[UART3], &higherPriorityTaskWoken);
    }
    if (INTGetFlag(INT_U3E)) {
        uartReg[UART3]->sta.clr = UART_PARITY_ERROR | UART_FRAMING_ERROR | UART_OVERRUN_ERROR;
        INTClearFlag(INT_U3E);
    }
    if (INTGetFlag(INT_U3TX)) {
        UART_TX_Handler(UART3);
        xSemaphoreGiveFromISR(txSemaphore[UART3], &higherPriorityTaskWoken);
        INTClearFlag(INT_U3TX);
    }
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}
#endif
#ifdef _UART4 //OR _UART1B
void UART_1B_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    if (INTGetFlag(INT_U4RX)) {
        UART_RX_Handler(UART4);
        INTClearFlag(INT_U4RX);
        xSemaphoreGiveFromISR(rxSemaphore[UART4], &higherPriorityTaskWoken);
    }
    if (INTGetFlag(INT_U4E)) {
        uartReg[UART4]->sta.clr = UART_PARITY_ERROR | UART_FRAMING_ERROR | UART_OVERRUN_ERROR;
        INTClearFlag(INT_U4E);
    }
    if (INTGetFlag(INT_U4TX)) {
        UART_TX_Handler(UART4);
        xSemaphoreGiveFromISR(txSemaphore[UART4], &higherPriorityTaskWoken);
        INTClearFlag(INT_U4TX);
    }
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}
#endif
#ifdef _UART5 //OR _UART 3B
void UART_3B_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    if (INTGetFlag(INT_U5RX)) {
        UART_RX_Handler(UART5);
        INTClearFlag(INT_U5RX);
        xSemaphoreGiveFromISR(rxSemaphore[UART5], &higherPriorityTaskWoken);
    }
    if (INTGetFlag(INT_U5E)) {
        uartReg[UART5]->sta.clr = UART_PARITY_ERROR | UART_FRAMING_ERROR | UART_OVERRUN_ERROR;
        INTClearFlag(INT_U5E);
    }
    if (INTGetFlag(INT_U5TX)) {
        UART_TX_Handler(UART5);
        xSemaphoreGiveFromISR(txSemaphore[UART5], &higherPriorityTaskWoken);
        INTClearFlag(INT_U5TX);
    }
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}
#endif
#ifdef _UART6 //OR _UART2B
void UART_2B_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    if (INTGetFlag(INT_U6RX)) {
        UART_RX_Handler(UART6);
        INTClearFlag(INT_U6RX);
        xSemaphoreGiveFromISR(rxSemaphore[UART6], &higherPriorityTaskWoken);
    }
    if (INTGetFlag(INT_U6E)) {
        uartReg[UART6]->sta.clr = UART_PARITY_ERROR | UART_FRAMING_ERROR | UART_OVERRUN_ERROR;
        INTClearFlag(INT_U6E);
    }
    if (INTGetFlag(INT_U6TX)) {
        UART_TX_Handler(UART6);
        xSemaphoreGiveFromISR(txSemaphore[UART6], &higherPriorityTaskWoken);
        INTClearFlag(INT_U6TX);
    }
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}
#endif