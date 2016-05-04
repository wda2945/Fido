/*
 * File:   uart.c
 * Author: martin
 *
 * Created on August 7, 2013, 8:19 PM
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "i2c.h"
#include "common.h"

int i2c_setup(const char *sclpin, const char *sdapin)
{
	bool pinmux_error = false;

	if (set_pinmux(sclpin, "i2c") < 0)
	{
		printf("set_pinmux(%s) fail\n", sclpin);
		pinmux_error = true;
	}
	if (set_pinmux(sdapin, "i2c") < 0)
	{
		printf("set_pinmux(%s) fail\n", sdapin);
		pinmux_error = true;
	}

    return (pinmux_error ? -1 : 0);
}

