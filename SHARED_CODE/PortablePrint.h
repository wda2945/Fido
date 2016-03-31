/***************************************************
	Platform Independent printf()
****************************************************/

#ifndef PORTABLE_PRINT_H
#define PORTABLE_PRINT_H

#ifdef __XC32__

#include "SysLog/SysLog.h"
    
#else

#include <stdio.h>

#define DebugPrint(...) printf(__VA_ARGS__)

#endif

#endif