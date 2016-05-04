//
//  Settings.h
//  Hex
//
//  Created by Martin Lane-Smith on 6/14/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

//settingmacro(name, var, min, max, def)

//navigator
settingmacro("Nav Loop mS", navLoopDelay, 100, 1000, 500)
settingmacro("Nav IMU Loop mS", imuLoopDelay, 100, 5000, 500)
settingmacro("Compass offset", compassOffset, 0, 360, 0)
settingmacro("Min HDOP", minHDOP, 0.5, 5, 1.5)
settingmacro("Nav App Report Interval S", appReportInterval, 1, 60, 10)
settingmacro("Nav Kalman Gain", KalmanGain, 0, 1, 0)
settingmacro("Nav Fix Wait S", FixWaitTime, 1, 30, 200)
settingmacro("Nav Latitude N", settingLatitude, 19, 20, 19.414617)
settingmacro("Nav Longitude W", settingLongitude, 154, 155, 154.891718)

//pilot
settingmacro("Pilot Loop mS", pilotLoopDelay, 100, 1000, 500)
settingmacro("Pilot Arrival Heading", arrivalHeading, 1, 30, 10)
settingmacro("Pilot Arrival Range", arrivalRange, 1, 30, 10)
settingmacro("Pilot FastSpeed", FastSpeed, 10, 100, 50)
settingmacro("Pilot MediumSpeed", MediumSpeed, 10, 100, 25)
settingmacro("Pilot SlowSpeed", SlowSpeed, 10, 100, 10)
settingmacro("Pilot Default Move cm", defMove, 10, 100, 50)
settingmacro("Pilot Default Turn deg", defTurn, 10, 100, 50)
settingmacro("Pilot Default Turnrate", defTurnRate, 10, 100, 50)
settingmacro("Pilot Motor Start TO", motorsStartTimeout, 1, 100, 5)
settingmacro("Pilot Motor TO/cm", timeoutPerCM, 0.1, 1, 0.5)
settingmacro("Pilot Motor Max cm", motorMaxCM, 10, 1000, 100)

//scanner
settingmacro("Scan proxweight", proxWeight, 0, 1, 0.5)
settingmacro("Scan focusweight", focusWeight, 0, 1, 0.5)
settingmacro("Scan fwdweight", fwdWeight, 0, 1, 0.25)
settingmacro("Scan ageweight", ageWeight, 0, 1, 0.75)
settingmacro("Scan contact wt", contactWeight, 0, 1, 0.5)
settingmacro("Scan close wt", closeWeight, 0, 1, 0.3)
settingmacro("Scan distant wt", distantWeight, 0, 1, 0.2)
//
settingmacro("BT Loop mS", behLoopDelay, 100, 1000, 250)
settingmacro("BT Ping Interval S", pingSecs, 5, 100, 5)
settingmacro("BT Network TO S", networkTimeout, 5, 100, 30)

settingmacro("APP Max Routine", maxUARTRoutine, 0, 100, 0)
settingmacro("APP Max Info", maxUARTInfo, 0, 100, 10)
settingmacro("APP Max Warning", maxUARTWarning, 0, 100, 10)
settingmacro("APP Max Error", maxUARTError, 0, 100, 50)
