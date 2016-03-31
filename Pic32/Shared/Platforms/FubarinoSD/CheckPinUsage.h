/* 
 * File:   CheckPinUsage.h
 * Author: martin
 *
 * Created on March 17, 2014, 2:26 PM
 */

#ifndef CHECKPINUSAGE_H
#define	CHECKPINUSAGE_H

//set remaining UARTS not in use
#ifndef USING_UART_1A
#define USING_UART_1A        0
#else
#define USING_PIN_8         1
#endif
#ifndef USING_UART_1A_RX
#define USING_UART_1A_RX     0
#endif
#ifndef USING_UART_1A_TX
#define USING_UART_1A_TX     0
#endif
#ifndef USING_UART_2A
#define USING_UART_2A        0
#else
#define USING_PIN_25        1
#endif
#ifndef USING_UART_2A_RX
#define USING_UART_2A_RX     0
#endif
#ifndef USING_UART_2A_TX
#define USING_UART_2A_TX     0
#endif
#ifndef USING_UART_3A
#define USING_UART_3A        0
#else
#define USING_PIN_31        1
#endif
#ifndef USING_UART_3A_RX
#define USING_UART_3A_RX     0
#endif
#ifndef USING_UART_3A_TX
#define USING_UART_3A_TX     0
#endif
#ifndef USING_UART_1B
#define USING_UART_1B        0
#else
#define USING_PIN_1         1
#endif
#ifndef USING_UART_1B_RX
#define USING_UART_1B_RX     0
#endif
#ifndef USING_UART_1B_TX
#define USING_UART_1B_TX     0
#endif
#ifndef USING_UART_2B
#define USING_UART_2B        0
#else
#define USING_PIN_27        1
#endif
#ifndef USING_UART_2B_RX
#define USING_UART_2B_RX     0
#endif
#ifndef USING_UART_2B_TX
#define USING_UART_2B_TX     0
#endif
#ifndef USING_UART_3B
#define USING_UART_3B        0
#else
#define USING_PIN_A7        1
#endif
#ifndef USING_UART_3B_RX
#define USING_UART_3B_RX     0
#endif
#ifndef USING_UART_3B_TX
#define USING_UART_3B_TX     0
#endif

//set remaining SPI ports not in use
#ifndef USING_SPI_2A
#define USING_SPI_2A         0
#else
#define USING_PIN_25         1
#endif
#ifndef USING_SPI_3A
#define USING_SPI_3A         0
#else
#define USING_PIN_28         1
#endif
#ifndef USING_SPI_1A
#define USING_SPI_1A         0
#else
#define USING_PIN_8          1
#endif

//set remaining I2C not in use
#ifndef USING_I2C_1
#define USING_I2C_1         0
#else
#define USING_PIN_1         1
#define USING_PIN_2         1
#endif
#ifndef USING_I2C_1A
#define USING_I2C_1A         0
#else
#define USING_PIN_8         1
#define USING_PIN_9         1
#endif
#ifndef USING_I2C_2A
#define USING_I2C_2A         0
#else
#define USING_PIN_25        1
#define USING_PIN_26        1
#endif
#ifndef USING_I2C_3A
#define USING_I2C_3A         0
#else
#define USING_PIN_28        1
#define USING_PIN_29        1
#endif

//remaining pins
#ifndef USING_PIN_0
#define USING_PIN_0             0
#endif
#ifndef USING_PIN_1
#define USING_PIN_1             0
#endif
#ifndef USING_PIN_2
#define USING_PIN_2             0
#endif
#ifndef USING_PIN_3
#define USING_PIN_3             0
#endif
#ifndef USING_PIN_4
#define USING_PIN_4             0
#endif
#ifndef USING_PIN_5
#define USING_PIN_5             0
#endif
#ifndef USING_PIN_6
#define USING_PIN_6             0
#endif
#ifndef USING_PIN_7
#define USING_PIN_7             0
#endif
#ifndef USING_PIN_8
#define USING_PIN_8             0
#endif
#ifndef USING_PIN_9
#define USING_PIN_9             0
#endif
#ifndef USING_PIN_10
#define USING_PIN_10            0
#endif
#ifndef USING_PIN_11
#define USING_PIN_11            0
#endif
#ifndef USING_PIN_12
#define USING_PIN_12            0
#endif
#ifndef USING_PIN_13
#define USING_PIN_13            0
#endif
#ifndef USING_PIN_14
#define USING_PIN_14            0
#endif
#ifndef USING_PIN_15
#define USING_PIN_15            0
#endif
#ifndef USING_PIN_16
#define USING_PIN_16            0
#endif
#ifndef USING_PIN_17
#define USING_PIN_17            0
#endif
#ifndef USING_PIN_18
#define USING_PIN_18            0
#endif
#ifndef USING_PIN_19
#define USING_PIN_19            0
#endif
#ifndef USING_PIN_20
#define USING_PIN_20            0
#endif
#ifndef USING_PIN_21
#define USING_PIN_21            0
#endif
#ifndef USING_PIN_22
#define USING_PIN_22            0
#endif
#ifndef USING_PIN_23
#define USING_PIN_23            0
#endif

#ifndef USING_PIN_24
#define USING_PIN_24            0
#endif
#ifndef USING_PIN_25
#define USING_PIN_25            0
#endif
#ifndef USING_PIN_26
#define USING_PIN_26            0
#endif
#ifndef USING_PIN_27
#define USING_PIN_27            0
#endif
#ifndef USING_PIN_A14
#define USING_PIN_A14           0
#endif
#ifndef USING_PIN_A13
#define USING_PIN_A13           0
#endif
#ifndef USING_PIN_A12
#define USING_PIN_A12           0
#endif
#ifndef USING_PIN_A11
#define USING_PIN_A11           0
#endif
#ifndef USING_PIN_A10
#define USING_PIN_A10           0
#endif
#ifndef USING_PIN_A9
#define USING_PIN_A9            0
#endif
#ifndef USING_PIN_A8
#define USING_PIN_A8            0
#endif
#ifndef USING_PIN_A7
#define USING_PIN_A7            0
#endif
#ifndef USING_PIN_A6
#define USING_PIN_A6            0
#endif
#ifndef USING_PIN_A5
#define USING_PIN_A5            0
#endif
#ifndef USING_PIN_A4
#define USING_PIN_A4            0
#endif
#ifndef USING_PIN_A3
#define USING_PIN_A3            0
#endif
#ifndef USING_PIN_A2
#define USING_PIN_A2            0
#endif
#ifndef USING_PIN_A1
#define USING_PIN_A1            0
#endif
#ifndef USING_PIN_A0
#define USING_PIN_A0            0
#endif
#ifndef USING_PIN_28
#define USING_PIN_28            0
#endif
#ifndef USING_PIN_29
#define USING_PIN_29            0
#endif

//check the use of SPI and UARTs
#if (USING_SPI_2A && (USING_UART_2A || USING_UART_2A_RX || USING_UART_2A_TX \
    || USING_UART_2B || USING_UART_2B_RX || USING_UART_2B_TX))
#error "Clash over SPI 2"
#endif
#if (USING_SPI_3A && (USING_UART_3A || USING_UART_3A_RX || USING_UART_3A_TX \
    || USING_UART_3B || USING_UART_3B_RX || USING_UART_3B_TX))
#error "Clash over SPI 1"
#endif
#if (USING_SPI_1A && (USING_UART_1A || USING_UART_1A_RX || USING_UART_1A_TX \
    || USING_UART_1B || USING_UART_1B_RX || USING_UART_1B_TX))
#error "Clash over SPI 3"
#endif

//check the use of I2C and UARTS and SPI
#if (USING_I2C_1 && (USING_UART_1B || USING_UART_1B_RX || USING_SPI_1A))
#error "Clash over I2C1"
#endif
#if (USING_I2C_3A && (USING_UART_3A || USING_UART_3A_RX  || USING_UART_3A_TX  || USING_SPI_3A))
#error "Clash over I2C3"
#endif
#if (USING_I2C_2A && (USING_UART_2A || USING_UART_2A_RX  || USING_UART_2A_TX || USING_SPI_2A))
#error "Clash over I2C4"
#endif
#if (USING_I2C_1A && (USING_UART_1B || USING_UART_1B_RX  || USING_UART_1B_TX || USING_SPI_1A))
#error "Clash over I2C5"
#endif

#endif	/* CHECKPINUSAGE_H */

