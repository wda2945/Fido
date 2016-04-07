/*
Copyright (c) 2013 Adafruit

Original RPi.GPIO Author Ben Croston
Modified for BBIO Author Justin Cooper

This file incorporates work covered by the following copyright and 
permission notice, all modified code adopts the original license:

Copyright (c) 2013 Ben Croston

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include "syslog/syslog.h"
#include "common.h"

#define PATHLEN 50

int build_path(const char *partial_path, const char *prefix, char *full_path, size_t full_path_len)
{
    DIR *dp;
    struct dirent *ep;

    dp = opendir (partial_path);
    if (dp != NULL) {
        while ((ep = readdir (dp))) {
            // Enforce that the prefix must be the first part of the file
            char* found_string = strstr(ep->d_name, prefix);

            if (found_string != NULL && (ep->d_name - found_string) == 0) {
                snprintf(full_path, full_path_len, "%s/%s", partial_path, ep->d_name);
                (void) closedir (dp);
                return 1;
            }
        }
        (void) closedir (dp);
    } else {
        return 0;
    }

    return 0;
}


int load_device_tree(const char *name)
{
    FILE *file = NULL;
    char line[256];
    char slots[PATHLEN];

//    char ctrl_dir[PATHLEN];
//    build_path("/sys/devices", "bone_capemgr", ctrl_dir, sizeof(ctrl_dir));
//    snprintf(slots, sizeof(slots), "%s/slots", ctrl_dir);
//
//    file = fopen(slots, "r+");
    file = fopen("/sys/devices/platform/bone_capemgr/slots", "r+");
    if (!file) {
        return 0;
    }

    while (fgets(line, sizeof(line), file)) {
        //the device is already loaded, return 1
        if (strstr(line, name)) {
            fclose(file);
            return 1;
        }
    }

    //if the device isn't already loaded, load it, and return
    fprintf(file,"%s",name);
    fclose(file);

    //0.2 second delay
    nanosleep((struct timespec[]){{0, 200000000}}, NULL);

    //check
    file = fopen(slots, "r+");
    if (!file) {
        return 0;
    }

    while (fgets(line, sizeof(line), file)) {
        //the device is loaded, return 1
        if (strstr(line, name)) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

int unload_device_tree(const char *name)
{
    FILE *file = NULL;
    char line[256];
    char slots[PATHLEN];
    char *slot_line;

//    char ctrl_dir[PATHLEN];
//    build_path("/sys/devices", "bone_capemgr", ctrl_dir, sizeof(ctrl_dir));
//    snprintf(slots, sizeof(slots), "%s/slots", ctrl_dir);
//
//    file = fopen(slots, "r+");
    file = fopen("/sys/devices/platform/bone_capemgr/slots", "r+");
    if (!file) {
        return 0;
    }

    while (fgets(line, sizeof(line), file)) {
        //the device is loaded, let's unload it
        if (strstr(line, name)) {
            slot_line = strtok(line, ":");
            //remove leading spaces
            while(*slot_line == ' ')
                slot_line++;

            fprintf(file, "-%s", slot_line);
            fclose(file);
            return 1;
        }
    }

    //not loaded, close file
    fclose(file);

    return 1;
}


int set_pinmux(const char *pinName, const char *newState)
{
	int fd;
    char pinmux_state_path[PATHLEN];	//pinmux path / state

    if (strlen(pinName) == 0) return 0;

    //3.8
//    char ocp_dir[PATHLEN];				//ocp root path
//    char pinmux[20];					//+ pinmux name
//    char pinmux_dir[PATHLEN];			//= pinmux path
//
//    //ocp directory path
//    build_path("/sys/devices", "ocp", ocp_dir, sizeof(ocp_dir));
//    //pinmux name
//    snprintf(pinmux, sizeof(pinmux), "%s_pinmux", pinName);
//    //pinmux path
//    build_path(ocp_dir, pinmux, pinmux_dir, sizeof(pinmux_dir));

//4.1
    //pinmux path
    char pinmux_dir[PATHLEN];			//= pinmux path
    snprintf(pinmux_dir, sizeof(pinmux_dir), "/sys/devices/platform/ocp/ocp:%s_pinmux", pinName);

    //set pinmux state
    snprintf(pinmux_state_path, sizeof(pinmux_state_path), "%s/state", pinmux_dir);
    if ((fd = open(pinmux_state_path, O_RDWR)) < 0)
    {
    	printf("open(%s) fail (%s)\n", pinmux_state_path, strerror(errno));
        return -1;
    }
    write(fd, newState,strlen(newState));
    close(fd);

    return 0;
}
