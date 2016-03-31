/* 
 * File:   I2C_Driver.h
 * Author: martin
 *
 * Created on March 26, 2014, 7:38 PM
 */

#ifndef I2C_DRIVER_H
#define	I2C_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define _DISABLE_OPENADC10_CONFIGPORT_WARNING
#define _SUPPRESS_PLIB_WARNING
#include "plib.h"

//I2C State Machine States
typedef enum {
    I2C_IDLE, I2C_SENDING_START, I2C_SENDING_RESTART, I2C_SENDING_STOP,
    I2C_SENDING_ADDRESS_READ, I2C_SENDING_ADDRESS_WRITE,
    I2C_READING_BYTE, I2C_SENDING_ACK, I2C_SENDING_NACK, I2C_WRITING_BYTE,
    I2C_START_COLLISION, I2C_COLLISION, I2C_NO_ACK, I2C_READ_OVERFLOW, I2C_TIMEOUT
} I2C_StateMachine_enum;

#define I2C_OK I2C_IDLE

#ifdef	__cplusplus
extern "C" {
#endif

//Init
bool I2C_Begin(I2C_MODULE module);

//Write address/W & N bytes
I2C_StateMachine_enum I2C_Write(I2C_MODULE _module, uint8_t address,
        uint8_t *data, uint8_t count);

//Write address/W & N bytes, then address/R and read M bytes
I2C_StateMachine_enum I2C_Read(I2C_MODULE _module, uint8_t address,
        uint8_t *writeData, uint8_t writeCount,
        uint8_t *readData, uint8_t readCount);

//Check State
I2C_StateMachine_enum I2C_getStatus(I2C_MODULE module);

//State to string
char *I2C_errorMsg(I2C_StateMachine_enum _code);

//end
bool I2C_Reset(I2C_MODULE _module);

//Added value
I2C_StateMachine_enum I2C_WriteRegister(I2C_MODULE _module, uint8_t address, uint8_t reg, uint8_t data);
I2C_StateMachine_enum I2C_ReadRegister(I2C_MODULE _module, uint8_t address, uint8_t reg, uint8_t *data);




#ifdef	__cplusplus
}
#endif

#endif	/* I2C_DRIVER_H */

