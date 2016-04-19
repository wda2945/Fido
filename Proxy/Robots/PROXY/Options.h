//
//  Options.h
//  Hex
//
//  Created by Martin Lane-Smith on 6/14/14.
//  Copyright (c) 2014 Martin Lane-Smith. All rights reserved.
//

//optionmacro(name, var, min, max, def)

optionmacro("Short Press", shortPress, 0, 1, 0)
optionmacro("Long Press", longPress, 0, 1, 0)
optionmacro("Loopback", loopback, 0, 1, 0)
optionmacro("Reset XBee", resetXBee, 0, 1, 0)
optionmacro("BatteryV", monitorBattery, 0, 1, 1)
optionmacro("FidoLED", monitorLED, 0, 1, 1)
optionmacro("Cyclic Sleep", cyclicSleep, 0, 1, 1)
