--[[
	SaveSettings,
	LoadSettings,
	NewWakeMask,
	WakeIn1Hour,
	WakeOnBumper,
	WakeOnIR,
	WakeOnPIR,
	WakeOnBatteryLow,
	WakeOnBatteryCritical,
	WakeOnChargingComplete,
	WakeAtSunset,
	WakeAtSunrise,
	WakeOnAppOnline,
    SystemPoweroff,
    SystemSleep,
    SystemWakeOnEvent,
    SystemSetResting,
    SystemSetActive,
    ReloadScripts,
    StrobeLightsOn,
    StrobeLightsOff,
    FrontLightsOn,
    FrontLightsOff,
    RearLightsOn,
    RearLightsOff,
    NavLightsOn,
    NavLightsOff,
    isCharging,
    isChargeComplete,
    isAppOnline,
    isRaining,
]]

BT.Task:new({
  name = 'SaveSettings',
  run = function(self, object)
  	result(SystemAction(system.SaveSettings))
  end
})

BT.Task:new({
  name = 'LoadSettings',
  run = function(self, object)
  	result(SystemAction(system.LoadSettings))
  end
})

BT.Task:new({
  name = 'NewWakeMask',
  run = function(self, object)
  	result(SystemAction(system.NewWakeMask))
  end
})

BT.Task:new({
  name = 'WakeIn1Hour',
  run = function(self, object)
  	result(SystemAction(system.WakeIn1Hour))
  end
})

BT.Task:new({
  name = 'WakeOnBumper',
  run = function(self, object)
  	result(SystemAction(system.WakeOnBumper))
  end
})

BT.Task:new({
  name = 'WakeOnIR',
  run = function(self, object)
  	result(SystemAction(system.WakeOnIR))
  end
})

BT.Task:new({
  name = 'WakeOnPIR',
  run = function(self, object)
  	result(SystemAction(system.WakeOnPIR))
  end
})

BT.Task:new({
  name = 'SaveActiveBehavior',
  run = function(self, object)
  	result(SystemAction(system.SaveActiveBehavior))
  end
})

BT.Task:new({
  name = 'WakeOnBatteryLow',
  run = function(self, object)
  	result(SystemAction(system.WakeOnBatteryLow))
  end
})

BT.Task:new({
  name = 'WakeOnBatteryCritical',
  run = function(self, object)
  	result(SystemAction(system.WakeOnBatteryCritical))
  end
})

BT.Task:new({
  name = 'WakeOnChargingComplete',
  run = function(self, object)
  	result(SystemAction(system.WakeOnChargingComplete))
  end
})

BT.Task:new({
  name = 'WakeAtSunset',
  run = function(self, object)
  	result(SystemAction(system.WakeAtSunset))
  end
})

BT.Task:new({
  name = 'WakeAtSunrise',
  run = function(self, object)
  	result(SystemAction(system.WakeAtSunrise))
  end
})

BT.Task:new({
  name = 'WakeOnAppOnline',
  run = function(self, object)
  	result(SystemAction(system.WakeOnAppOnline))
  end
})

BT.Task:new({
  name = 'SystemPoweroff',
  run = function(self, object)
  	result(SystemAction(system.SystemPoweroff))
  end
})

BT.Task:new({
  name = 'SystemSleep',
  run = function(self, object)
  	result(SystemAction(system.SystemSleep))
  end
})

BT.Task:new({
  name = 'SystemWakeOnEvent',
  run = function(self, object)
  	result(SystemAction(system.SystemWakeOnEvent))
  end
})

BT.Task:new({
  name = 'SystemSetResting',
  run = function(self, object)
  	result(SystemAction(system.SystemSetResting))
  end
})

BT.Task:new({
  name = 'SystemSetActive',
  run = function(self, object)
  	result(SystemAction(system.SystemSetActive))
  end
})

BT.Task:new({
  name = 'ReloadScripts',
  run = function(self, object)
  	result(SystemAction(system.ReloadScripts))
  end
})

BT.Task:new({
  name = 'StrobeLightsOn',
  run = function(self, object)
  	result(SystemAction(system.StrobeLightsOn))
  end
})

BT.Task:new({
  name = 'StrobeLightsOff',
  run = function(self, object)
  	result(SystemAction(system.StrobeLightsOff))
  end
})

BT.Task:new({
  name = 'FrontLightsOn',
  run = function(self, object)
  	result(SystemAction(system.FrontLightsOn))
  end
})

BT.Task:new({
  name = 'FrontLightsOff',
  run = function(self, object)
  	result(SystemAction(system.FrontLightsOff))
  end
})

BT.Task:new({
  name = 'RearLightsOn',
  run = function(self, object)
  	result(SystemAction(system.RearLightsOn))
  end
})

BT.Task:new({
  name = 'RearLightsOff',
  run = function(self, object)
  	result(SystemAction(system.RearLightsOff))
  end
})

BT.Task:new({
  name = 'NavLightsOn',
  run = function(self, object)
  	result(SystemAction(system.NavLightsOn))
  end
})

BT.Task:new({
  name = 'NavLightsOff',
  run = function(self, object)
  	result(SystemAction(system.NavLightsOff))
  end
})

BT.Task:new({
  name = 'isCharging',
  run = function(self, object)
	result(SystemAction(system.isCharging))
  end
})

BT.Task:new({
  name = 'isChargeComplete',
  run = function(self, object)
	result(SystemAction(system.isChargeComplete))
  end
})

BT.Task:new({
  name = 'isRaining',
  run = function(self, object)
	result(SystemAction(system.isRaining))
  end
})
