/*
 * File:   I2C_Driver.c
 * Author: martin
 *
 * Created on March 26, 2014, 7:38 PM
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "SoftwareProfile.h"
#include "HardwareProfile.h"
#include "SysLog/SysLog.h"

#include "Drivers/I2C/I2C.h"

#define I2C_SEMPHR_WAIT     100

//semaphors for ISRs
SemaphoreHandle_t masterSemaphore[I2C_NUMBER_OF_MODULES];

//struct for I2C requests
typedef struct {
    uint8_t address;        //slave address
    uint8_t *writeData;     //write buffer address
    uint8_t writeCount;     //bytes to write
    uint8_t *readData;      //read buffer address
    uint8_t readCount;      //bytes to read
    I2C_7_BIT_ADDRESS slaveWriteAddress, slaveReadAddress;
    I2C_StateMachine_enum i2cState;
    I2C_StateMachine_enum i2cError;
} I2Cdata_t;

I2Cdata_t i2cData[I2C_NUMBER_OF_MODULES];

INT_SOURCE i2cMasterInterrupt[I2C_NUMBER_OF_MODULES] = {
#ifdef _I2C1
    INT_I2C1M,
#endif
#ifdef _I2C2
    INT_I2C2M,
#endif
#ifdef _I2C3
    INT_I2C3M,
#endif
#ifdef _I2C4
    INT_I2C4M,
#endif
#ifdef _I2C5
    INT_I2C5M,
#endif
};
INT_VECTOR i2cMasterVector[I2C_NUMBER_OF_MODULES] = {
#ifdef _I2C1
    INT_I2C_1_VECTOR,
#endif
#ifdef _I2C2
    INT_I2C_2_VECTOR,
#endif
#ifdef _I2C3
    INT_I2C_3_VECTOR,
#endif
#ifdef _I2C4
    INT_I2C_4_VECTOR,
#endif
#ifdef _I2C5
    INT_I2C_5_VECTOR,
#endif
};
INT_SOURCE i2cSlaveInterrupt[I2C_NUMBER_OF_MODULES] = {
#ifdef _I2C1
    INT_I2C1S,
#endif
#ifdef _I2C2
    INT_I2C2S,
#endif
#ifdef _I2C3
    INT_I2C3S,
#endif
#ifdef _I2C4
    INT_I2C4S,
#endif
#ifdef _I2C5
    INT_I2C5S,
#endif
};
INT_SOURCE i2cBusInterrupt[I2C_NUMBER_OF_MODULES] = {
#ifdef _I2C1
    INT_I2C1B,
#endif
#ifdef _I2C2
    INT_I2C2B,
#endif
#ifdef _I2C3
    INT_I2C3B,
#endif
#ifdef _I2C4
    INT_I2C4B,
#endif
#ifdef _I2C5
    INT_I2C5B,
#endif
};

bool I2C_Begin(I2C_MODULE _module) {

    if (_module >= I2C_NUMBER_OF_MODULES) return false;

    //disable all interrupts while configuring
    INTEnable(i2cMasterInterrupt[_module], INT_DISABLED);
    INTEnable(i2cSlaveInterrupt[_module], INT_DISABLED);
    INTEnable(i2cBusInterrupt[_module], INT_DISABLED);

    //Disable the I2C module
    I2CEnable(_module, FALSE);

    //create the master semaphore
    if (masterSemaphore[_module] == NULL)
    {
        masterSemaphore[_module] = xSemaphoreCreateBinary();
        if (masterSemaphore[_module] == NULL) {
            LogError( "i2cSemphr");
            return false;
        }
    }

    //configure module
    // Set the I2C baudrate
    UINT32 actualClock = I2CSetFrequency(_module, GetPeripheralClock(), I2C_CLOCK_FREQ);
    if (abs(actualClock - I2C_CLOCK_FREQ) > I2C_CLOCK_FREQ / 10) {
        //not set to frequency needed (> 10% error)?
    }
    // Enable the I2C module
    I2CEnable(_module, TRUE);

    /* Set the interrupt privilege level and sub-privilege level */
    INTSetVectorPriority(i2cMasterVector[_module], I2C_INT_PRIORITY);
    INTSetVectorSubPriority(i2cMasterVector[_module], I2C_INT_SUB_PRIORITY);

    i2cData[_module].i2cError = i2cData[_module].i2cState = I2C_IDLE;
    return true;
}

//common starting procedure

I2C_StateMachine_enum IC2_StartOperation(I2C_MODULE _module) {

    I2C_FORMAT_7_BIT_ADDRESS(i2cData[_module].slaveWriteAddress, i2cData[_module].address, I2C_WRITE);
    I2C_FORMAT_7_BIT_ADDRESS(i2cData[_module].slaveReadAddress, i2cData[_module].address, I2C_READ);

    //wait for the bus to be idle
    while (!I2CBusIsIdle(_module));

    //sending start
    i2cData[_module].i2cState = I2C_SENDING_START;
    i2cData[_module].i2cError = I2C_OK;
//    INTClearFlag(i2cMasterInterrupt[_module]);
    //Enable the interrupt
    INTEnable(i2cMasterInterrupt[_module], INT_ENABLED);

    if (I2CStart(_module) != I2C_SUCCESS) {
        //bus collision!
        I2C_Reset(_module);
        return (i2cData[_module].i2cError = I2C_START_COLLISION);
    }

    //wait for completion interrupt
    if (xSemaphoreTake(masterSemaphore[_module], I2C_SEMPHR_WAIT) == pdTRUE) {
        i2cData[_module].i2cError = i2cData[_module].i2cState;
        //chack the state
        switch (i2cData[_module].i2cState) {
            case I2C_COLLISION:
                I2C_Reset(_module);
                return I2C_COLLISION;
                break;
            case I2C_NO_ACK:
                i2cData[_module].i2cState = I2C_IDLE;
                return I2C_NO_ACK;
                break;
            case I2C_READ_OVERFLOW:
                //clear the flag
                I2CClearStatus(_module, I2C_RECEIVER_OVERFLOW);
                i2cData[_module].i2cState = I2C_IDLE;
                return I2C_READ_OVERFLOW;
                break;
            default:
                return I2C_OK;
                break;
        }
    } else {
        //timeout
//        I2C_Reset(_module);
        return (i2cData[_module].i2cError = I2C_TIMEOUT);
    }
}

//write N bytes to the device
I2C_StateMachine_enum I2C_Write(I2C_MODULE _module, uint8_t _address, uint8_t *_writeData, uint8_t _writeCount) {
    i2cData[_module].address = _address;
    i2cData[_module].writeData = _writeData;
    i2cData[_module].writeCount = _writeCount;
    i2cData[_module].readData = 0;
    i2cData[_module].readCount = 0;

    return IC2_StartOperation(_module);
}
//write N bytes, then read M bytes
I2C_StateMachine_enum I2C_Read(I2C_MODULE _module, uint8_t _address, uint8_t *_writeData, uint8_t _writeCount,
        uint8_t *_readData, uint8_t _readCount) {

    i2cData[_module].address = _address;
    i2cData[_module].writeData = _writeData;
    i2cData[_module].writeCount = _writeCount;
    i2cData[_module].readData = _readData;
    i2cData[_module].readCount = _readCount;

    return IC2_StartOperation(_module);
}
I2C_StateMachine_enum I2C_getStatus(I2C_MODULE _module)
{
    return i2cData[_module].i2cError;
}
//reset the bus
bool I2C_Reset(I2C_MODULE _module) {
    //disable all interrupts while configuring
    INTEnable(i2cMasterInterrupt[_module], INT_DISABLED);
    INTEnable(i2cSlaveInterrupt[_module], INT_DISABLED);
    INTEnable(i2cBusInterrupt[_module], INT_DISABLED);
    INTClearFlag(i2cMasterInterrupt[_module]);
    //Disable the I2C module
    I2CEnable(_module, FALSE);
    vTaskDelay(1000);
    //Re-enable the I2C module
    I2CEnable(_module, TRUE);
    i2cData[_module].i2cState = I2C_IDLE;
    return true;
}
//error state to text
char *I2C_errorMsg(I2C_StateMachine_enum _code)
{
    switch(_code)
    {
        case I2C_START_COLLISION:
            return "Start Coll";
            break;
        case I2C_COLLISION:
            return "Collision";
            break;
        case I2C_NO_ACK:
            return "No ACK";
            break;
        case I2C_READ_OVERFLOW:
            return "Overflow";
            break;
        case I2C_TIMEOUT:
            return "Timeout";
            break;
        default:
            return "OK";
            break;
    }
}

I2C_StateMachine_enum I2C_WriteRegister(I2C_MODULE _module, uint8_t _address, uint8_t _reg, uint8_t _data)
{
    uint8_t buffer[2];
    buffer[0] = _reg;
    buffer[1] = _data;
    return I2C_Write(_module, _address, &buffer[0], 2);
}
I2C_StateMachine_enum I2C_ReadRegister(I2C_MODULE _module, uint8_t _address, uint8_t _reg, uint8_t *_data)
{
    return I2C_Read(_module, _address, &_reg, 1, _data, 1);
}
/************************************************/
//interrupt ISRs
//Assembly wrappers
#ifdef USING_I2C_1
void __attribute__((interrupt(I2C_IPL), vector(_I2C_1_VECTOR))) I2C_1_ISR_Wrapper(void);
#endif
#ifdef USING_I2C_2
void __attribute__((interrupt(I2C_IPL), vector(_I2C_2_VECTOR))) I2C_2_ISR_Wrapper(void);
#endif
#ifdef USING_I2C_3
void __attribute__((interrupt(I2C_IPL), vector(_I2C_3_VECTOR))) I2C_3_ISR_Wrapper(void);
#endif
#ifdef USING_I2C_4
void __attribute__((interrupt(I2C_IPL), vector(_I2C_4_VECTOR))) I2C_4_ISR_Wrapper(void);
#endif
#ifdef USING_I2C_5
void __attribute__((interrupt(I2C_IPL), vector(_I2C_5_VECTOR))) I2C_5_ISR_Wrapper(void);
#endif

//main handler called by individual handlers
bool I2C_Master_Handler(I2C_MODULE _module) {
    I2C_STATUS  status;
    I2Cdata_t *thisModule = &i2cData[_module];
    //PORTToggleBits(PIN_18_IOPORT, PIN_18_BIT);

    //figure interrupt reason:
    switch (thisModule->i2cState) {
        case I2C_SENDING_START:
            status = I2CGetStatus(_module);
            if ((status & I2C_START) && I2CTransmitterIsReady(_module))
            {
                //Start Condition done
                if (thisModule->writeCount) {
                    //  Send slave address byte w/Write
                    I2CSendByte(_module, thisModule->slaveWriteAddress.byte);
                    thisModule->i2cState = I2C_SENDING_ADDRESS_WRITE;
                } else if (thisModule->readCount) {
                    //  Send slave address byte w/Read
                    I2CSendByte(_module, thisModule->slaveReadAddress.byte);
                    thisModule->i2cState = I2C_SENDING_ADDRESS_READ;
                } else {
                    //nothing to do!
                    I2CStop(_module);
                    thisModule->i2cState = I2C_SENDING_STOP;
                }
            }
            break;
        case I2C_SENDING_ADDRESS_WRITE:
        case I2C_WRITING_BYTE:
            //PORTToggleBits(PIN_17_IOPORT, PIN_17_BIT);
            //check ACK
            status = I2CGetStatus(_module);
            if (I2CByteWasAcknowledged(_module)) {
                //send next write byte, if any, or Restart or Stop
                if (thisModule->writeCount--) {
                    I2CSendByte(_module, *(thisModule->writeData++));
                    thisModule->i2cState = I2C_WRITING_BYTE;
                } else {
                    if (thisModule->readCount) {
                        I2CRepeatStart(_module);
                        thisModule->i2cState = I2C_SENDING_RESTART;
                    } else {
                        I2CStop(_module);
                        thisModule->i2cState = I2C_SENDING_STOP;
                    }
                }
            } else {
                I2CStop(_module);
                thisModule->i2cState = I2C_NO_ACK;
            }
            break;
        case I2C_SENDING_RESTART:
            status = I2CGetStatus(_module);
            if ((status & I2C_START) && I2CTransmitterIsReady(_module)) {
                //Repeated Start Sequence done
                //  Send slave address byte w/Read
                I2CSendByte(_module, thisModule->slaveReadAddress.byte);
                thisModule->i2cState = I2C_SENDING_ADDRESS_READ;
            }
            break;

        case I2C_SENDING_ADDRESS_READ:

            //check ACK
            status = I2CGetStatus(_module);
            if (I2CByteWasAcknowledged(_module)) {
                //Initiate Read byte
                if (thisModule->readCount--) {
                    if (I2CReceiverEnable(_module, true) == I2C_SUCCESS) {
                        thisModule->i2cState = I2C_READING_BYTE;
                    } else {
                        thisModule->i2cState = I2C_READ_OVERFLOW;
                        I2CClearStatus(_module, I2C_RECEIVER_OVERFLOW);
                        I2CStop(_module);
                    }
                } else {
                    I2CStop(_module);
                    thisModule->i2cState = I2C_SENDING_STOP;
                }

            } else {
                I2CStop(_module);
                thisModule->i2cState = I2C_NO_ACK;
            }
            break;
        case I2C_READING_BYTE:
            //PORTToggleBits(PIN_16_IOPORT, PIN_16_BIT);
            //Data transfer byte received
            *(thisModule->readData++) = I2CGetByte(_module);
            //  Send ACK if more expected, else NAK
            if (thisModule->readCount--) {
                I2CAcknowledgeByte(_module, true);
                thisModule->i2cState = I2C_SENDING_ACK;
            } else {
                I2CAcknowledgeByte(_module, false);
                thisModule->i2cState = I2C_SENDING_NACK;
            }
            break;
        case I2C_SENDING_ACK:
            //During a Send ACK sequence
            //  Initiate a read
            if (I2CReceiverEnable(_module, true) == I2C_SUCCESS) {
                thisModule->i2cState = I2C_READING_BYTE;
            } else {
                thisModule->i2cState = I2C_READ_OVERFLOW;
                I2CClearStatus(_module, I2C_RECEIVER_OVERFLOW);
                I2CStop(_module);
            }
            break;
        case I2C_SENDING_NACK:
            I2CStop(_module);
            thisModule->i2cState = I2C_SENDING_STOP;
            break;
        case I2C_SENDING_STOP:
            thisModule->i2cState = I2C_IDLE;
            //disable interrupt
            INTEnable(i2cMasterInterrupt[_module], INT_DISABLED);
            return true; //Sequence complete
            break;
            //error exits
        case I2C_NO_ACK:
        case I2C_READ_OVERFLOW:
            INTEnable(i2cMasterInterrupt[_module], INT_DISABLED);
            return true; //Sequence complete - error
            break;
        default:
            break;

            //During a slave-detected Stop?

    }
    return false;
}

bool I2C_Bus_Handler(int device) {
    //do nothing?
    return false;
}

//ISRs for each I2C channel
#ifdef USING_I2C_1
void I2C_1_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    if (INTGetFlag(INT_I2C1M)) {
        INTClearFlag(INT_I2C1M);
        if (I2C_Master_Handler(I2C1)) //handle a Master interrupt
            xSemaphoreGiveFromISR(masterSemaphore[I2C1], &higherPriorityTaskWoken);
    } else if (INTGetFlag(INT_I2C1S)) {
        INTClearFlag(INT_I2C1S); //clear a slave interrupt
    } else if (INTGetFlag(INT_I2C1B)) {
        I2C_Bus_Handler(I2C1); //handle a Bus interrupt
        INTClearFlag(INT_I2C1B);
    }
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}
#endif

#ifdef USING_I2C_2
void I2C_2_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    if (INTGetFlag(INT_I2C2M)) {
        INTClearFlag(INT_I2C2M);
        if (I2C_Master_Handler(I2C2)) //handle a Master interrupt
            xSemaphoreGiveFromISR(masterSemaphore[I2C2], &higherPriorityTaskWoken);
    } else if (INTGetFlag(INT_I2C2S)) {
        INTClearFlag(INT_I2C2S); //clear a slave interrupt
    } else if (INTGetFlag(INT_I2C2B)) {
        I2C_Bus_Handler(I2C2); //handle a Bus interrupt
        INTClearFlag(INT_I2C2B);
    }
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}
#endif

#ifdef USING_I2C_3
void I2C_3_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    if (INTGetFlag(INT_I2C3M)) {
        if (I2C_Master_Handler(I2C3))
            xSemaphoreGiveFromISR(masterSemaphore[I2C3], &higherPriorityTaskWoken);
        INTClearFlag(INT_I2C3M);
    } else if (INTGetFlag(INT_I2C3S)) {
        INTClearFlag(INT_I2C3S);
    } else if (INTGetFlag(INT_I2C3B)) {
        I2C_Bus_Handler(I2C3);
        INTClearFlag(INT_I2C3B);
    }
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}
#endif

#ifdef USING_I2C_4

void I2C_4_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    if (INTGetFlag(INT_I2C4M)) {
        if (I2C_Master_Handler(I2C4))
            xSemaphoreGiveFromISR(masterSemaphore[I2C4], &higherPriorityTaskWoken);
        INTClearFlag(INT_I2C4M);
    } else if (INTGetFlag(INT_I2C4S)) {
        INTClearFlag(INT_I2C4S);
    } else if (INTGetFlag(INT_I2C4B)) {
        I2C_Bus_Handler(I2C4);
        INTClearFlag(INT_I2C4B);
    }
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}
#endif

#ifdef USING_I2C_5

void I2C_5_ISR_Handler() {
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
    if (INTGetFlag(INT_I2C5M)) {
        if (I2C_Master_Handler(I2C5))
            xSemaphoreGiveFromISR(masterSemaphore[I2C5], &higherPriorityTaskWoken);
        INTClearFlag(INT_I2C5M);
    } else if (INTGetFlag(INT_I2C5S)) {
        INTClearFlag(INT_I2C5S);
    } else if (INTGetFlag(INT_I2C5B)) {
        I2C_Bus_Handler(I2C5);
        INTClearFlag(INT_I2C5B);
    }
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}

#endif
