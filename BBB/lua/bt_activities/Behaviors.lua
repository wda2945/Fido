
-- common battery check
BT.Priority:new({
	name = 'isBatteryOKish',		-- battery not critical; OK or low
	nodes = {
		'isBatteryOK',
		'isBatteryLow',
		'BatteryFailAlert'
		}
});

--go to sunbathe
Sunbathe = BT:new({
	tree = BT.Sequence:new({
		name = 'Go.To.Solar.Position',
		nodes = {
			'isBatteryOKish',
			'WaitForFix',
			'isPilotReady',
			'ComputeSolarPosition',
			'DriveAndAvoidObstacle'
		}
	})
});

ActivityList[NextActivity] =  'Sunbathe'
NextActivity = NextActivity + 1

--return to heel
Heel = BT:new({
	tree = BT.Sequence:new({
		name = 'Go.To.Heel.Position',
		nodes = {
			'isBatteryOKish',
			'WaitForFix',
			'isPilotReady',
			'ComputeHeelPosition',
			'DriveAndAvoidObstacle'
		}
	})
});

ActivityList[NextActivity] =  'Heel'
NextActivity = NextActivity + 1

--return home
Home = BT:new({
	tree = BT.Sequence:new({
		name = 'Go.To.Home.Position',
		nodes = {
			'isBatteryOKish',
			'WaitForFix',
			'isPilotReady',
			'ComputeHomePosition',
			'DriveAndAvoidObstacle'
		}
	})
});

ActivityList[NextActivity] =  'Home'
NextActivity = NextActivity + 1

Explore = BT:new({
	tree = BT.RepeatWhileSuccess:new({
		name = 'Random.Explore',
		nodes = {
			BT.Priority:new({
				name = 'BatteryOKalert',
				nodes = {
					'isBatteryOK',
					'BatteryFailAlert'
					}
			}),
			'WaitForFix',
			'isPilotReady',
			'ComputeRandomExplorePosition',
			'DriveAndAvoidObstacle',
			'Wait60'
		}
	})
});

ActivityList[NextActivity] =  'Explore'
NextActivity = NextActivity + 1

ReloadAllScripts = BT:new({
  tree = 'ReloadScripts'
})

ActivityList[NextActivity] =  'ReloadAllScripts'
NextActivity = NextActivity + 1

PowerOff = BT:new({
  tree = 'SystemPoweroff'
})

ActivityList[NextActivity] =  'PowerOff'
NextActivity = NextActivity + 1

Sleep = BT:new({
  tree = 'SystemSleep'
})

ActivityList[NextActivity] =  'Sleep'
NextActivity = NextActivity + 1