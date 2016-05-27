//
//  Settings.h
//  Hex
//
//  Created by Martin Lane-Smith on 6/14/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

//settingmacro(name, var, min, max, def)

settingmacro("Power Level", powerLevel, 0, 4, 3)
settingmacro("Max LogInfo", maxAgentInfo, 0, 100, 50)
settingmacro("Max LogWarn", maxAgentWarning, 0, 100, 50)
settingmacro("Max LogError", maxAgentError, 0, 100, 100)
settingmacro("Short Press mS", shortPressS, 100, 500, 250)
settingmacro("Long Press mS", longPressS, 500, 2000, 1000)
