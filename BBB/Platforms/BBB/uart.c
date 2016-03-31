/*
 * File:   uart.c
 * Author: martin
 *
 * Created on August 7, 2013, 8:19 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "uart.h"
#include "common.h"

int uart_setup(const char *txpin, const char *rxpin)
{
	if (set_pinmux(txpin, "uart") < 0)
	{
		printf("set_pinmux(%s) fail", txpin);
		return -1;
	}
	if (set_pinmux(rxpin, "uart") < 0)
	{
		printf("set_pinmux(%s) fail", rxpin);
		return -1;
	}

    return 0;
}

