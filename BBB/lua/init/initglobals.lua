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

activate = function(activity_name, activity_table)
	CurrentActivity = activity_table
	CurrentActivityName = activity_name
	;(CurrentActivity):setObject(activity_name);
end

update = function()
	if CurrentActivity then
		activityResult = (CurrentActivity):run(CurrentActivityName)
		
		if activityResult == 'success' or activityResult == 'fail' then
			CurrentActivity = Idle;		
		end
		if activityResult then Print('Update - ' .. activityResult) else Print('Update ?') end
		return activityResult
	else
		CurrentActivity = Idle;		
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