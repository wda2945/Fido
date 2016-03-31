--[[
	DriveAndAvoidObstacle assumes a waypoint has been set and engages the autopilot.
		If the autopilot fails, evasive maneuvers are attempted, N times.
		Succeeds if at waypoint
		Fails if battery critical or repeat counts out
]]

BT.AlwaysSucceedDecorator:new({				--don't care whether or not an evasive maneuver actually finsihes
	name = 'MoveBackwardAlwaysSucceed',
  	node = 'MoveBackward'
})

BT.AlwaysSucceedDecorator:new({
	name = 'TurnAlwaysSucceed',
  	node = 'Turn'
})

BT.AlwaysSucceedDecorator:new({
	name = 'TurnLeftAlwaysSucceed',
  	node = 'TurnLeft'
})

BT.AlwaysSucceedDecorator:new({
	name = 'TurnRightAlwaysSucceed',
  	node = 'TurnRight'
})

BT.Sequence:new({
	name = 'DriveAndAvoidObstacle',
	nodes = {
		BT.RepeatWhileFail:new({			-- right now, retries for ever!
		name = 'Retry.Avoid.Obstacle',
			nodes = {
				BT.Priority:new({
				  name = 'Avoid.Obstacle',
				  nodes = {
					BT.Sequence:new({		-- the happy sequence
						name = 'Orient.Engage',
						nodes = {
						'EnableFrontContactStop',
						'EnableRearContactStop',
						'Orient',
						'DisableRearContactStop',
						'EnableFrontCloseStop',
						'Engage'
						}						-- nodes = {
					}),							-- BT.Sequence:new({
		
					BT.AlwaysFailDecorator:new({
						node = BT.Sequence:new({
							name = 'Front.Contact',	-- first possible reason for fail
							nodes = {
								'isFrontContact',
								'DiableFrontContactStop',
								'DisableFrontCloseStop',
								'EnableRearContactStop',
								'MoveBackwardAlwaysSucceed',
								'EnableFrontContactStop',
								'TurnAlwaysSucceed',
								'DisableRearContactStop',
								'MoveForward'
								}				-- nodes = {
							})					-- BT.Sequence:new({
					}),							-- BT.AlwaysFailDecorator:new({
		
					BT.AlwaysFailDecorator:new({
						node = BT.Sequence:new({
							name = 'Front.Left.Contact',  -- second possible reason, etc.
							nodes = {
								'isFrontLeftContact',
								'DiableFrontContactStop',
								'DisableFrontCloseStop',
								'EnableRearContactStop',
								'MoveBackwardAlwaysSucceed',
								'EnableFrontContactStop',
								'TurnRightAlwaysSucceed',
								'DisableRearContactStop',
								'MoveForward'
								}				-- nodes = {
							})					-- BT.Sequence:new({
					}),							-- BT.AlwaysFailDecorator:new({
		
					BT.AlwaysFailDecorator:new({
						node = BT.Sequence:new({
							name = 'Front.Right.Contact',
							nodes = {
								'isFrontRightContact',
								'DiableFrontContactStop',
								'DisableFrontCloseStop',
								'EnableRearContactStop',
								'MoveBackwardAlwaysSucceed',
								'EnableFrontContactStop',
								'TurnLeftAlwaysSucceed',
								'DisableRearContactStop',
								'MoveForward'
								}				-- nodes = {
							})					-- BT.Sequence:new({
					}),							-- BT.AlwaysFailDecorator:new({
		
					BT.AlwaysFailDecorator:new({
						node = BT.Sequence:new({
							name = 'Front.Prox',
							nodes = {
								'isFrontProximity',
								'EnableFrontContactStop',
								'EnableRearContactStop',
								'DisableFrontCloseStop',
								'TurnAlwaysSucceed',
								'DisableRearContactStop',
								'EnableFrontCloseStop',
								'MoveForward'
								}				-- nodes = {
							})					-- BT.Sequence:new({
					}),							-- BT.AlwaysFailDecorator:new({
	
					BT.AlwaysFailDecorator:new({
						node = BT.Sequence:new({
							name = 'Front.Left.Prox',
							nodes = {
								'isFrontLeftProximity',
								'EnableFrontContactStop',
								'EnableRearContactStop',
								'DisableFrontCloseStop',
								'TurnRightAlwaysSucceed',
								'DisableRearContactStop',
								'EnableFrontCloseStop',
								'MoveForward'
								}				-- nodes = {
							})					-- BT.Sequence:new({
					}),							-- BT.AlwaysFailDecorator:new({
		
					BT.AlwaysFailDecorator:new({
						node = BT.Sequence:new({
							name = 	'Front.Right.Prox',
							nodes = {
								'isFrontRightProximity',
								'EnableFrontContactStop',
								'EnableRearContactStop',
								'DisableFrontCloseStop',
								'TurnLeftAlwaysSucceed',
								'DisableRearContactStop',
								'EnableFrontCloseStop',
								'MoveForward'
								}				-- nodes = {
							})					-- BT.Sequence:new({
					}),							-- BT.AlwaysFailDecorator:new({
			
					'isAtWaypoint',				-- break out of the loop if we're there
					'isBatteryCritical'			-- break out if the battery's flat!
				}								-- nodes = {
			}),									-- BT.Priority:new({
		
			'isAtWaypoint'						-- return success only if we're there
			}									-- nodes = {
		})										-- BT.RepeatWhileFail:new({
	}											-- nodes = {
})												-- BT.Sequence:new({
