/* 
 * File: NotificationsEventsList.h
 * Author: martin
 *
 * Created on April 9, 2015, 8:46 PM
 */

//Notifications - events

EVENT( NULL_EVENT, "Null")
EVENT( FRONT_CONTACT_EVENT, "OnFrontContact")
EVENT( REAR_CONTACT_EVENT,  "OnRearContact")
EVENT( BATTERY_SHUTDOWN_EVENT, "OnBatteryShutdown")
EVENT( BATTERY_CRITICAL_EVENT, "OnBatteryCritical")
EVENT( BATTERY_LOW_EVENT, "OnBatteryLow")
EVENT( SLEEPING_EVENT, "OnSleeping")
EVENT( ONE_HOUR_EVENT, "In1Hour")   //1 hour after sleeping
EVENT( PROXIMITY_EVENT, "OnProximity")
EVENT( PIR_EVENT, "OnPIR")
EVENT( LOST_FIX_EVENT, "OnLostFix")
EVENT( CHARGING_COMPLETE_EVENT, "OnChargingComplete")
EVENT( SUNSET_EVENT, "AtSunset")
EVENT( SUNRISE_EVENT, "AtSunrise")
EVENT( APPONLINE_EVENT, "OnAppOnline")
