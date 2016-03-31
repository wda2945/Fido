/* 
 * File:   Mapping.h
 * Author: martin
 *
 * Helper functions for Mapping
 *
 */
#include <math.h>
#include "Helpers.h"

//conversion for pos
#define LEFT_LONGITUDE 		-154.89252f /*-154.8925f   		bounds of map */
#define TOP_LATITUDE 		 19.41535f /*19.4155f */
#define MAP_DEGREES_TO_CM	(185200.0 * 60.0)   /*(185200.0 * 60.0)	cm per nm x 60 minutes per degree*/
#define LAT_ADJUSTMENT 		(cosf((19.4155f * M_PI) / 180.0))


//Mapping
//northing and easting are cm from datum
float LatitudeToNorthing(float latitude)
{
	float deltaN = (TOP_LATITUDE - latitude);
	return (deltaN * MAP_DEGREES_TO_CM);
}
float LongitudeToEasting(float longitude)
{
	float deltaE = (longitude - LEFT_LONGITUDE);
	return (deltaE * MAP_DEGREES_TO_CM * LAT_ADJUSTMENT);
}
float NorthingToLatitude(float northing)
{
	return (TOP_LATITUDE - (northing / MAP_DEGREES_TO_CM));
}
float EastingToLongitude(float easting)
{
	return (LEFT_LONGITUDE + (easting / (MAP_DEGREES_TO_CM * LAT_ADJUSTMENT)));
}
