--local BT = require('/root/lua/behaviortree/behaviour_tree')

--[[
	Wait10
	Wait60
	
	ActivityFailAlert
	BatteryFailAlert
	ActivityCompleteAlert
	ObstacleFailAlert
	
]]

local startTime

BT.Task:new({
  name = 'Wait10',			-- 10 seconds
  start = function(self, object)
  	startTime = secondCount
  	Print('Wait10...')
  end,
  run = function(self, object)
    if (startTime + 10) > secondCount then
  		Print('...done')
    	success()
    else 
    	running()
    end
  end
})

BT.Task:new({
  name = 'Wait60',			-- 60 seconds
  start = function(self, object)
  	Print('Wait60...')
  	startTime = secondCount
  end,
  run = function(self, object)
    if (startTime + 60) > secondCount then
  		Print('...done')
    	success()
    else 
    	running()
    end
  end
})

BT.Task:new({
  name = 'ActivityFailAlert',
  run = function(self, object)
  	Alert(object .. ': Fail')
    fail()
  end
})

BT.Task:new({
  name = 'BatteryFailAlert',
  run = function(self, object)
  	Alert(object .. ': Battery Fail')
    fail()
  end
})

BT.Task:new({
  name = 'ActivityCompleteAlert',
  run = function(self, object)
  	Alert(object .. ': Complete')
    success()
  end
})

BT.Task:new({
  name = 'ObstacleFailAlert',
  run = function(self, object)
  	Alert(object .. ': Obstacle Fail')
    fail()
  end
})
