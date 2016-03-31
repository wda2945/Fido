--[[
	isFrontLeftContact,
	isFrontContact,
	isFrontRightContact,
	isRearLeftContact,
	isRearContact,
	isRearRightContact,
	isFrontLeftProximity,
	isFrontProximity,
	isFrontRightProximity,
	isRearLeftProximity,
	isRearProximity,
	isRearRightProximity,
	isLeftProximity,
	isRightProximity,
	isFrontLeftFarProximity,
	isFrontFarProximity,
	isFrontRightFarProximity,
	isLeftPassiveProximity,
	isRightPassiveProximity,
	EnableInfrared,
	DisableInfrared,
	EnableSonar,
	DisableSonar,
	EnablePassive,
	DisablePassive,
]]

BT.Task:new({
  name = 'isFrontLeftContact',
  run = function(self, object)
  	result(ProximityAction(proximity.isFrontLeftContact))
  end
})
			
	
BT.Task:new({
  name = 'isFrontContact',
  run = function(self, object)
  	result(ProximityAction(proximity.isFrontContact))
  end
})
	
BT.Task:new({
  name = 'isFrontRightContact',
  run = function(self, object)
  	result(ProximityAction(proximity.isFrontRightContact))
  end
})
	
BT.Task:new({
  name = 'isRearLeftContact',
  run = function(self, object)
  	result(ProximityAction(proximity.isRearLeftContact))
  end
})
	
BT.Task:new({
  name = 'isRearContact',
  run = function(self, object)
  	result(ProximityAction(proximity.isRearContact))
  end
})
	
BT.Task:new({
  name = 'isRearRightContact',
  run = function(self, object)
  	result(ProximityAction(proximity.isRearRightContact))
  end
})
	
BT.Task:new({
  name = 'isFrontLeftProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isFrontLeftProximity))
  end
})
	
BT.Task:new({
  name = 'isFrontProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isFrontProximity))
  end
})
	
BT.Task:new({
  name = 'isFrontRightProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isFrontRightProximity))
  end
})
	
BT.Task:new({
  name = 'isRearLeftProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isRearLeftProximity))
  end
})
	
BT.Task:new({
  name = 'isRearProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isRearProximity))
  end
})
	
BT.Task:new({
  name = 'isRearRightProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isRearRightProximity))
  end
})
	
BT.Task:new({
  name = 'isLeftProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isLeftProximity))
  end
})
	
BT.Task:new({
  name = 'isRightProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isRightProximity))
  end
})
	
BT.Task:new({
  name = 'isFrontLeftFarProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isFrontLeftFarProximity))
  end
})
	
BT.Task:new({
  name = 'isFrontFarProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isFrontFarProximity))
  end
})
	
BT.Task:new({
  name = 'isFrontRightFarProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isFrontRightFarProximity))
  end
})
	
BT.Task:new({
  name = 'isLeftPassiveProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isLeftPassiveProximity))
  end
})
	
BT.Task:new({
  name = 'isRightPassiveProximity',
  run = function(self, object)
  	result(ProximityAction(proximity.isRightPassiveProximity))
  end
})

BT.Task:new({
  name = 'EnableInfrared',
  run = function(self, object)
  	result(ProximityAction(proximity.EnableInfrared))
  end
})

BT.Task:new({
  name = 'DisableInfrared',
  run = function(self, object)
  	result(ProximityAction(proximity.DisableInfrared))
  end
})

BT.Task:new({
  name = 'EnableSonar',
  run = function(self, object)
  	result(ProximityAction(proximity.EnableSonar))
  end
})

BT.Task:new({
  name = 'DisableSonar',
  run = function(self, object)
  	result(ProximityAction(proximity.DisableSonar))
  end
})

BT.Task:new({
  name = 'EnablePassive',
  run = function(self, object)
  	result(ProximityAction(proximity.EnablePassive))
  end
})

BT.Task:new({
  name = 'DisablePassive',
  run = function(self, object)
  	result(ProximityAction(proximity.DisablePassive))
  end
})

	