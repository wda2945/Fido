-- First globals to be set. 
-- Behavior Trees add themselves to the ActivityList
-- This provides the Behavior menu to the App

ActivityList = {}
NextActivity = 1

Idle = BT:new({
  tree = BT.Task:new({
  			name = 'IdleTask',
  			run = function(task, object)
  				-- Print(object .. ' Task run()')
				success()
 				end
 			});
});

ActivityList[NextActivity] =  'Idle'
NextActivity = NextActivity + 1

CurrentActivity = Idle;
CurrentActivityName = 'Idle'

-- called when the App selects a Behavior Tree
activate = function(activity_name, activity_table)
	CurrentActivity = activity_table
	CurrentActivityName = activity_name
	;(CurrentActivity):setObject(CurrentActivityName);
end

-- called periodically
update = function()
	if CurrentActivity then
		activityResult = (CurrentActivity):run(CurrentActivityName)
		
		if activityResult == 'success' or activityResult == 'fail' then
--			Print('Update - ' .. activityResult)
			activate('IdleTask', Idle)	
		else
			if activityResult == 'running' then 
--				Print('Update - running')
			else
--				Print('Update invalid result: ' .. activityResult) 
				return 'invalid'
			end	
		end
		return activityResult
	else
		activate('IdleTask', Idle)		
		return 'invalid'
	end
end
	
secondCount = 0

result = function(reply)
	if reply == 'success' then
		--Print('LUA Success')
    	success()
    elseif reply == 'running' then
    	running()
	else
		--Print('LUA Fail')
    	fail()
    end
end