/*
 ============================================================================
 Name        : GPS.c
 Author      : Martin
 Version     :
 Copyright   : (c) 2015 Martin Lane-Smith
 Description : Reads and parses GPS
 ============================================================================
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "uart.h"
#include "common.h"
#include "softwareprofile.h"
#include "pubsubdata.h"
#include "pubsub/pubsub.h"

#include "syslog/syslog.h"

#include "GPS.h"

FILE *gpsDebugFile;

#ifdef GPS_DEBUG
#define DEBUGPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(gpsDebugFile, __VA_ARGS__);fflush(gpsDebugFile);
#else
#define DEBUGPRINT(...) fprintf(gpsDebugFile, __VA_ARGS__);fflush(gpsDebugFile);
#endif

#define ERRORPRINT(...) LogError(__VA_ARGS__);fprintf(gpsDebugFile, __VA_ARGS__);fflush(gpsDebugFile);


void *GPSReaderThread(void *arg);
int ParseNMEA();
uint8_t parseHex(char c);
void SendCommand(const unsigned char *buffer);

char currentline[MAXLINELENGTH];
// our index into filling the line
uint8_t lineidx = 0;
int GPSfd;

uint8_t hour, minute, seconds, year, month, day;
uint16_t milliseconds;
float latitude, longitude, geoidheight, altitude;
float speed, angle, magvariation, HDOP;
char lat, lon, mag;
bool fix;
uint8_t fixquality, satellites;

int GPSInit()
{
	gpsDebugFile = fopen("/root/logfiles/gps.log", "w");

	//open GPS uart
	struct termios settings;

	if (load_device_tree(GPS_UART_OVERLAY) < 0)
	{
		ERRORPRINT("GPS uart overlay: %s failed\n", GPS_UART_OVERLAY);
		return -1;
	}
	else {
		DEBUGPRINT("GPS uart overlay OK\n", PS_UART_DEVICE);
	}

	if (uart_setup(GPS_TX_PIN, GPS_RX_PIN) < 0)
	{
		ERRORPRINT("GPS uart_setup failed: %s\n");
		return -1;
	}
	else {
		DEBUGPRINT("broker uart pinmux OK\n");
	}

	sleep(1);

	//initialize UART
	GPSfd = open(GPS_UART_DEVICE, O_RDWR | O_NOCTTY);

	int retryCount = 10;

	while (GPSfd < 0 && retryCount-- > 0) {
		ERRORPRINT("Open %s: %s\n",GPS_UART_DEVICE, strerror(errno));
		sleep(1);
		GPSfd = open(GPS_UART_DEVICE, O_RDWR | O_NOCTTY);
	}

	if (GPSfd < 0) {
		return -1;
	}
	else
	{
		DEBUGPRINT("GPS %s opened\n", GPS_UART_DEVICE);
	}

	if (tcgetattr(GPSfd, &settings) != 0) {
		ERRORPRINT("GPS tcgetattr failed: %s\n", strerror(errno));
		return -1;
	}

	//no processing
	settings.c_iflag = 0;
	settings.c_oflag = 0;
	settings.c_lflag = 0;
	settings.c_cflag = CLOCAL | CREAD | CS8;        //no modem, 8-bits

	//baudrate
	cfsetospeed(&settings, GPS_UART_BAUDRATE);
	cfsetispeed(&settings, GPS_UART_BAUDRATE);

	if (tcsetattr(GPSfd, TCSANOW, &settings) != 0) {
		ERRORPRINT("GPS tcsetattr failed: %s\n", strerror(errno));
		return -1;
	}

	DEBUGPRINT("GPS uart configured\n");

	//Create thread
	pthread_t thread;
	int s = pthread_create(&thread, NULL, GPSReaderThread, NULL);
	if (s != 0)
	{
		ERRORPRINT("GPSReaderThread create failed. %i\n", s);
		return -1;
	}
	return 0;
}

//thread to parse GPS
void *GPSReaderThread(void *arg)
{
	psMessage_t msg;

    // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
     SendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
     // uncomment this line to turn on only the "minimum recommended" data
     //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
     // Set the update rate
     SendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
     // Request updates on antenna status, comment out to keep quiet
     SendCommand(PGCMD_ANTENNA);
     //need to spend time processing GPS data

    DEBUGPRINT("GPS thread ready\n");

	while (1)
	{
		//wait for complete frame from GPS
        int parseResult = false;
        lineidx = 0;
        //read a message
        while (!parseResult) {
            char c;
            int chars_read;
            do
            {
            	chars_read = read(GPSfd, &c, 1);
            } while (chars_read == 0);

            if (c == '$') {
                lineidx = 0;
                currentline[lineidx] = c;
            }
            if (c == '\n') {
                currentline[lineidx] = 0;
                lineidx = 0;

                parseResult = ParseNMEA();
            } else {

                currentline[lineidx++] = c;
                if (lineidx >= MAXLINELENGTH) {
                    lineidx = MAXLINELENGTH - 1;
                    currentline[lineidx] = 0;
                    ParseNMEA();
                }
            }

        }
	}
}

//Parse NMEA message

int ParseNMEA() {

    psMessage_t msg;
    uint8_t i;
    //Have an NMEA message to parse

    DEBUGPRINT("GPS: %s\n", currentline);

    // do checksum check

    // first look if we even have one
    if (currentline[strlen(currentline) - 4] == '*') {
        uint16_t sum = parseHex(currentline[strlen(currentline) - 3]) * 16;
        sum += parseHex(currentline[strlen(currentline) - 2]);

        // check checksum
        for (i = 1; i < (strlen(currentline) - 4); i++) {
            sum ^= currentline[i];
        }
        if (sum != 0) {
            // bad checksum :(
            return false;
        }
    }

    // look for a few common sentences
    if (strstr(currentline, "$GPGGA")) {
        // found GGA
        char *p = currentline;
        // get time
        p = strchr(p, ',') + 1;
        float timef = atof(p);
        uint32_t time = timef;
        hour = time / 10000;
        minute = (time % 10000) / 100;
        seconds = (time % 100);

        milliseconds = fmod(timef, 1.0) * 1000;

        // parse out latitude
        p = strchr(p, ',') + 1;
        latitude = atof(p);

        p = strchr(p, ',') + 1;
        if (p[0] == 'N') lat = 'N';
        else if (p[0] == 'S') lat = 'S';
        else if (p[0] == ',') lat = 0;
        else return false;

        // parse out longitude
        p = strchr(p, ',') + 1;
        longitude = atof(p);

        p = strchr(p, ',') + 1;
        if (p[0] == 'W') lon = 'W';
        else if (p[0] == 'E') lon = 'E';
        else if (p[0] == ',') lon = 0;
        else return false;

        p = strchr(p, ',') + 1;
        fixquality = atoi(p);

        p = strchr(p, ',') + 1;
        satellites = atoi(p);

        p = strchr(p, ',') + 1;
        HDOP = atof(p);

        p = strchr(p, ',') + 1;
        altitude = atof(p);
        p = strchr(p, ',') + 1;
        p = strchr(p, ',') + 1;
        geoidheight = atof(p);
    } else if (strstr(currentline, "$GPRMC")) {
        // found RMC
        char *p = currentline;

        // get time
        p = strchr(p, ',') + 1;
        float timef = atof(p);
        uint32_t time = timef;
        hour = time / 10000;
        minute = (time % 10000) / 100;
        seconds = (time % 100);

        milliseconds = fmod(timef, 1.0) * 1000;

        p = strchr(p, ',') + 1;
        // Serial.println(p);
        if (p[0] == 'A')
            fix = true;
        else if (p[0] == 'V')
            fix = false;
        else
            return false;

        // parse out latitude
        p = strchr(p, ',') + 1;
        latitude = atof(p);

        p = strchr(p, ',') + 1;
        if (p[0] == 'N') lat = 'N';
        else if (p[0] == 'S') lat = 'S';
        else if (p[0] == ',') lat = 0;
        else return false;

        // parse out longitude
        p = strchr(p, ',') + 1;
        longitude = atof(p);

        p = strchr(p, ',') + 1;
        if (p[0] == 'W') lon = 'W';
        else if (p[0] == 'E') lon = 'E';
        else if (p[0] == ',') lon = 0;
        else return false;

        // speed
        p = strchr(p, ',') + 1;
        speed = atof(p);

        // angle
        p = strchr(p, ',') + 1;
        angle = atof(p);

        p = strchr(p, ',') + 1;
        uint32_t fulldate = atof(p);
        day = fulldate / 10000;
        month = (fulldate % 10000) / 100;
        year = (fulldate % 100);


    }// we don't parse the remaining, yet!
    else return false;

    /////////////////////////// FIX

    float longMinutes = longitude - 15400.0;
    longitude = -154.0 - (longMinutes / 60.0);

    float latMinutes = latitude - 1900.0;
    latitude = 19.0 + (latMinutes / 60.0);

    /////////////////////////// FIX

    //send a message

    DEBUGPRINT("GPS Report: %s @ %f N, %f W\n", (fix > 0 ? "Fix" : "No Fix"), latitude, longitude);

    psInitPublish(msg, GPS_REPORT);
    msg.positionPayload.gpsStatus = (fix > 0 ? 1 : 0);
//    msg.positionPayload.seconds = seconds;
//    msg.positionPayload.hour = hour;
//    msg.positionPayload.minute = minute;
//    msg.positionPayload.seconds = seconds;
//    msg.positionPayload.year = year;
//    msg.positionPayload.month = month;
//    msg.positionPayload.day = day;
    msg.positionPayload.latitude = latitude;
    msg.positionPayload.longitude = longitude;
    msg.positionPayload.HDOP = HDOP;

    RouteMessage(&msg);

//    hour = minute = seconds = year = month = day =
//            fixquality = satellites = 0; // uint8_t
//    lat = lon = mag = 0; // char
//    milliseconds = 0; // uint16_t
//    latitude = longitude = geoidheight = altitude =
//            speed = angle = magvariation = HDOP = 0.0; // float

    return true;
}

// read a Hex value and return the decimal equivalent

uint8_t parseHex(char c) {
    if (c < '0')
        return 0;
    if (c <= '9')
        return c - '0';
    if (c < 'A')
        return 0;
    if (c <= 'F')
        return (c - 'A') + 10;
}

void SendCommand(const unsigned char *buffer) {
	int len = strlen(buffer);
    write(GPSfd, buffer, len);
}

