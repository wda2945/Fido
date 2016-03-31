//
//  Settings.h
//  MOTOR
//
//  Created by Martin Lane-Smith on 3/25/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//


settingmacro("Duty Starting",startingDuty,0.1,0.5,0.3)
settingmacro("Duty Increment",dutyIncrement,0.0,0.5,0.1)
//settingmacro("PID Deadband",deadband,0,500,500)

settingmacro("Duty Min",minDuty,0.1,0.5,0.3)
settingmacro("Duty Max",maxDuty,0.3,1.0,0.75)

settingmacro("PID P Coeff",pCoefficient,0,0.5,0.05)
//settingmacro("PID I Coeff",iCoefficient,0,0.1,0.0005)
//settingmacro("PID D Coeff",dCoefficient,0,0.1,0)
settingmacro("PID Interval mS",pidInterval,10,500,200)
settingmacro("Odo Interval mS",odoInterval,500,5000,1000)
        
settingmacro("Comm Stats Interval S", commStatsSecs, 5, 600, 60)
settingmacro("Notify Min Interval mS", notifyMinInterval, 100, 5000, 500)

settingmacro("Max Routine", maxUARTRoutine, 0, 100, 0)
settingmacro("Max Info", maxUARTInfo, 0, 100, 50)
settingmacro("Max Warning", maxUARTWarning, 0, 100, 50)
settingmacro("Max Error", maxUARTError, 0, 100, 50)