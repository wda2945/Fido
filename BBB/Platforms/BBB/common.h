/*
 * File:   common.h
 * Author: martin
 *
 */

#ifndef COMMON_H
#define	COMMON_H

//build a path finding the version number
int build_path(const char *partial_path, const char *prefix, char *full_path, size_t full_path_len);

//write name to /sys/devices/bone_capemgr.*/slots
int load_device_tree(const char *name);
int unload_device_tree(const char *name);

//write newState to <pin>pinmux
int set_pinmux(const char *pinName, const char *newState);

#endif
