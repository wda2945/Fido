//
//  Settings.h
//  Hex
//
//  Created by Martin Lane-Smith on 6/14/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

//settingmacro(name, var, min, max, def)

settingmacro("Power Level", powerLevel, 0, 4, 3)
settingmacro("Max LogInfo", maxAgentInfo, 0, 100, 0)
settingmacro("Max LogWarn", maxAgentWarning, 0, 100, 0)
settingmacro("Max LogError", maxAgentError, 0, 100, 10)
//settingmacro("Remote Poll S", remotePollS, 5, 100, 10)
settingmacro("Short Press mS", shortPressS, 100, 500, 250)
settingmacro("Long Press S", longPressS, 500, 2000, 1000)
//settingmacro("Cyclic Sleep S", cyclicSleepPeriodS, 5, 60, 15)
//settingmacro("Time Before Sleep S", timeToSleepS, 5, 120, 30)
