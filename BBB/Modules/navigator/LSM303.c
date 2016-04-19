/*
 ============================================================================
 Name        : LSM303.c
 Author      : Martin
 Version     :
 Copyright   : (c) 2015 Martin Lane-Smith
 Description : Reads IMU
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
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "i2c.h"
#include "LSM303.h"
#include "IMU.h"
#include "syslog/syslog.h"

#define LOGFILEPRINT(...)  fprintf(imuDebugFile, __VA_ARGS__);fflush(imuDebugFile);
#define ERRORPRINT(...) LogError(__VA_ARGS__);fprintf(imuDebugFile, __VA_ARGS__);fflush(imuDebugFile);

#ifdef DEBUGSTDOUT
#define DEBUGPRINT(...) fprintf(stdout, __VA_ARGS__);fprintf(imuDebugFile, __VA_ARGS__);fflush(imuDebugFile);
#else
#define DEBUGPRINT(...)
#endif

///////////////////////////////////////////////

    vector_int16_t m_max // maximum magnetometer values, used for calibration
   			= {+2259,  +2170,  +2175};
     vector_int16_t m_min // minimum magnetometer values, used for calibration
    		= {-1653,  -1792,  -1480};
    uint8_t last_status; // status of last I2C transmission

    void LSM303_writeReg(uint8_t reg, uint8_t value);
    uint8_t LSM303_readReg(uint8_t reg);

    unsigned int io_timeout = 0;
    bool did_timeout = false;

bool LSM303_timeoutOccurred()
{
  bool tmp = did_timeout;
  did_timeout = false;
  return tmp;
}

void LSM303_setTimeout(unsigned int timeout)
{
  io_timeout = timeout;
}

unsigned int LSM303_getTimeout()
{
  return io_timeout;
}

/*
Enables the LSM303's accelerometer and magnetometer. Also:
- Sets sensor full scales (gain) to default power-on values, which are
  +/- 2 g for accelerometer and +/- 1.3 gauss for magnetometer
  (+/- 4 gauss on LSM303D).
- Selects 50 Hz ODR (output data rate) for accelerometer and 7.5 Hz
  ODR for magnetometer (6.25 Hz on LSM303D). (These are the ODR
  settings for which the electrical characteristics are specified in
  the datasheets.)
- Enables high resolution modes (if available).
Note that this function will also reset other settings controlled by
the registers it writes to.
*/
void LSM303_enableDefault(void)
{
    // Accelerometer

    // 0x00 = 0b00000000
    // AFS = 0 (+/- 2 g full scale)
	LSM303_writeReg(CTRL2, 0x00);

    // 0x57 = 0b01010111
    // AODR = 0101 (50 Hz ODR); AZEN = AYEN = AXEN = 1 (all axes enabled)
	LSM303_writeReg(CTRL1, 0x57);

    // Magnetometer

    // 0x64 = 0b01100100
    // M_RES = 11 (high resolution mode); M_ODR = 001 (6.25 Hz ODR)
	LSM303_writeReg(CTRL5, 0x64);

    // 0x20 = 0b00100000
    // MFS = 01 (+/- 4 gauss full scale)
	LSM303_writeReg(CTRL6, 0x20);

    // 0x00 = 0b00000000
    // MLP = 0 (low power mode off); MD = 00 (continuous-conversion mode)
	LSM303_writeReg(CTRL7, 0x00);
}

// Writes a register
void LSM303_writeReg(uint8_t reg, uint8_t value)
{
	last_status = i2c_smbus_write_byte_data(IMU_FD, reg, value);
	if (last_status < 0)
	{
		ERRORPRINT("i2c_smbus_write_byte_data to imu %x fail - %s\n", reg, strerror(errno));
	}
}

// Reads a register
uint8_t LSM303_readReg(uint8_t reg)
{
	last_status = i2c_smbus_read_byte_data(IMU_FD, reg);
	if (last_status < 0)
	{
		ERRORPRINT("i2c_smbus_read_byte_data to imu %x fail - %s\n", reg, strerror(errno));
		return last_status;
	}
	return (last_status & 0xff);
}

// Reads the 3 accelerometer channels and stores them in vector a
void LSM303_readAcc(void)
{
	int i;
	uint8_t regData[6];

	for (i=0; i<6; i++)
	{
		regData[i] = LSM303_readReg(OUT_X_L_A + i);
	}

  // combine high and low bytes
  // This no longer drops the lowest 4 bits of the readings from the DLH/DLM/DLHC, which are always 0
  // (12-bit resolution, left-aligned). The D has 16-bit resolution
  LSM303_a.x = (int16_t)(regData[1] << 8 | regData[0]);
  LSM303_a.y = (int16_t)(regData[3] << 8 | regData[2]);
  LSM303_a.z = (int16_t)(regData[5] << 8 | regData[4]);
}

// Reads the 3 magnetometer channels and stores them in vector m
void LSM303_readMag(void)
{
	int i;
	uint8_t regData[6];

	for (i=0; i<6; i++)
	{
		regData[i] = LSM303_readReg(OUT_X_L_M + i);
	}
  // combine high and low bytes
	  LSM303_m.x = (int16_t)(regData[1] << 8 | regData[0]);
	  LSM303_m.y = (int16_t)(regData[3] << 8 | regData[2]);
	  LSM303_m.z = (int16_t)(regData[5] << 8 | regData[4]);
}

// Reads all 6 channels of the LSM303 and stores them in the object variables
void LSM303_read(void)
{
	LSM303_readAcc();
	LSM303_readMag();
}

void vector_normalize(vector_float *a)
{
  float mag = sqrt(vector_dot(a, a));
  a->x /= mag;
  a->y /= mag;
  a->z /= mag;
}

//////////////////
/*
Returns the angular difference in the horizontal plane between the
"from" vector and north, in degrees.

Description of heading algorithm:
Shift and scale the magnetic reading based on calibration data to find
the North vector. Use the acceleration readings to determine the Up
vector (gravity is measured as an upward acceleration). The cross
product of North and Up vectors is East. The vectors East and North
form a basis for the horizontal plane. The From vector is projected
into the horizontal plane and the angle between the projected vector
and horizontal north is returned.
*/
float LSM303_heading()
{
	vector_float from = {0,0,1};			//Z axis forward
	vector_float temp_m = {LSM303_m.x, LSM303_m.y, LSM303_m.z};
	vector_float temp_a = {LSM303_a.x, LSM303_a.y, LSM303_a.z};

    // subtract offset (average of min and max) from magnetometer readings
    temp_m.x -= ((float)m_min.x + m_max.x) / 2;
    temp_m.y -= ((float)m_min.y + m_max.y) / 2;
    temp_m.z -= ((float)m_min.z + m_max.z) / 2;

    // compute E and N
    vector_float E;
    vector_float N;
    vector_cross(&temp_m, &temp_a, &E);
    vector_normalize(&E);
    vector_cross(&temp_a, &E, &N);
    vector_normalize(&N);

    // compute heading
    float heading = atan2(vector_dot(&E, &from), vector_dot(&N, &from)) * 180 / M_PI;
    if (heading < 0) heading += 360;
    return heading;
}

float LSM303_pitch(void)
{
	return atan2(LSM303_a.z, LSM303_a.x) * 180 / M_PI;
}
float LSM303_roll(void)
{
	return atan2(LSM303_a.y, LSM303_a.x) * 180 / M_PI;
}

void vector_cross(const vector_float *a, const vector_float *b, vector_float *out)
{
  out->x = (a->y * b->z) - (a->z * b->y);
  out->y = (a->z * b->x) - (a->x * b->z);
  out->z = (a->x * b->y) - (a->y * b->x);
}

float vector_dot(const vector_float *a, const vector_float *b)
{
  return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}
