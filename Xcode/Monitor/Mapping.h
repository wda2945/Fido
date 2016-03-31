/* 
 * File:   Mapping.h
 * Author: martin
 *
 * Helper functions for Messages
 *
 */
 
#ifndef _MAPPING_H_
#define _MAPPING_H_

#ifdef __cplusplus
extern "C" {
#endif

//Mapping
//northing and easting are cm from datum
float LatitudeToNorthing(float latitude);
float LongitudeToEasting(float longitude);
float NorthingToLatitude(float northing);
float EastingToLongitude(float easting);


#ifdef __cplusplus
}
#endif

#endif //_MAPPING_H_
