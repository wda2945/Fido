/* 
 * File:   Settings.h
 * Author: martin
 *
 * MCP
 *
 * Created on April 15, 2014, 11:40 AM
 */

//settingmacro(name, var, min, max, def)

settingmacro("Env Report Interval mS", envReport, 500, 60000, 10000)
settingmacro("Prox Report Interval mS", proxReportIntervalMs, 500, 60000, 5000)

//IR prox thresholds
settingmacro("Prox Max Range IR cm", maxRangeIR, 1, 50, 30)
settingmacro("Prox Hit Duration mS", proxHitDetection, 1000, 10000, 500)
settingmacro("Prox Miss Duration mS", proxMissDetection, 1000, 10000, 2000)
        
//Bumper FSR
settingmacro("FSR Thresh", fsrThresh, 0.1, 0.9, 0.75)

//min range for sonar hit
settingmacro("Prox Min Range Sonar cm", minRangeSonar, 20, 1000, 30)
settingmacro("Prox Max Range Sonar cm", maxRangeSonar, 20, 1000, 200)

//battery checks
settingmacro("Battery Low V", battLowV, 15, 25, 24)
settingmacro("Battery Critical V", battCriticalV, 15, 25, 22)
settingmacro("Battery Shutdown V", shutdownV, 15, 25, 20)
settingmacro("Battery Motor Offset V", motorOffsetV, 0, 10, 2)
settingmacro("Battery Low Ah", battLowAh, 0, 10, 1.44)          //80%
settingmacro("Battery Critical Ah", battCriticalAh, 0, 10, 2.5) //65%
settingmacro("Battery Shutdown Ah", shutdownAh, 0, 10, 3.6)     //50%
settingmacro("Battery Low Count", battLowCount, 1, 5, 20)
settingmacro("Battery Capacity", battCapacity, 5, 10, 7.2)
        
//BBB power timers
settingmacro("OVM Poweroff Timeout S", poweroffTimeout, 1, 100, 30)
settingmacro("OVM Powerup Timeout S", powerupTimeout, 1, 100, 30)
settingmacro("OVM PwrBtn Delay S", pwrBtnDelay, 0, 10, 2)
settingmacro("OVM PwrBtn Length S", pwrBtnLength, 0.5, 10, 2)

settingmacro("CommStats Interval S", commStatsSecs, 5, 600, 60)
settingmacro("Strobe Interval mS", strobeInterval, 200, 2000, 400)       
settingmacro("Notify Min Interval mS", notifyMinInterval, 100, 5000, 500)
        
settingmacro("BLE Max Routine", maxAPPRoutine, 0, 100, 0)
settingmacro("BLE Max Info", maxAPPInfo, 0, 100, 5)
settingmacro("BLE Max Warning", maxAPPWarning, 0, 100, 5)
settingmacro("BLE Max Error", maxAPPError, 0, 100, 10)

settingmacro("OVM Max Routine", maxUARTRoutine, 0, 100, 0)
settingmacro("OVM Max Info", maxUARTInfo, 0, 100, 50)
settingmacro("OVM Max Warning", maxUARTWarning, 0, 100, 50)
settingmacro("OVM Max Error", maxUARTError, 0, 100, 50)

settingmacro("XBEE Max Routine", maxXBeeRoutine, 0, 100, 0)
settingmacro("XBEE Max Info", maxXBeeInfo, 0, 100, 50)
settingmacro("XBEE Max Warning", maxXBeeWarning, 0, 100, 50)
settingmacro("XBEE Max Error", maxXBeeError, 0, 100, 50)
settingmacro("XBEE Power Level", powerLevel, 0, 4, 4)