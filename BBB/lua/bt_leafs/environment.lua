--[[
	isBatteryCritical
	isBatteryLow
	isBatteryOK
]]
				
BT.Task:new({
  name = 'isBatteryCritical',
  run = function(self, object)
    if battery.status == battery.critical then
     	print('Battery Critical Success')
     	success()
    else 
    	print('Battery Critical Fail')
    	fail()
    end
  end
})
				
BT.Task:new({
  name = 'isBatteryLow',
  run = function(self, object)
    if battery.status == battery.low then
     	print('Battery Low Success')
   		success()
    else 
		print('Battery Low Fail')
    	fail()
    end
  end
})

				
BT.Task:new({
  name = 'isBatteryOK',
  run = function(self, object)
    --Print('BattOK control = ' .. self.control.name)
	if battery.status == battery.nominal or battery.status == battery.unknown then
    	print('Battery OK Success')
    	success()
    else 
    	print('Battery OK Fail')
    	Fail('isBatteryOK')
    	fail()
    end
  end
})


