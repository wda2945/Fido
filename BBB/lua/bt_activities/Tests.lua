
Test = BT:new({
  name = 'TestTree',
  tree = BT.Sequence:new({
	name = 'Test.Seq',
		nodes = {
			'DisableFrontContactStop',
			'DisableFrontCloseStop',
			'DisableRearContactStop',
			'DisableRearCloseStop',
			'MoveForward',
			'TurnLeft',
			'MoveBackward',
			'TurnRight'
			}
		})
});

ActivityList[NextActivity] =  'Test'
NextActivity = NextActivity + 1

Test90 = BT:new({
  name = 'Test.Turn.Left.90',
  tree = BT.Sequence:new({
	name = 'Test.Seq',
		nodes = {
			'DisableFrontContactStop',
			'DisableFrontCloseStop',
			'DisableRearContactStop',
			'DisableRearCloseStop',
			'TurnLeft90',
			}
		})
});

ActivityList[NextActivity] =  'Test90'
NextActivity = NextActivity + 1

TestNorth = BT:new({
  name = 'Test.Turn.North',
  tree = BT.Sequence:new({
	name = 'Test.Seq',
		nodes = {
			'DisableFrontContactStop',
			'DisableFrontCloseStop',
			'DisableRearContactStop',
			'DisableRearCloseStop',
			'TurnNorth',
			}
		})
});

ActivityList[NextActivity] =  'TestNorth'
NextActivity = NextActivity + 1

TestMove10 = BT:new({
  name = 'Test.Move.10',
  tree = BT.Sequence:new({
	name = 'Test.Seq',
		nodes = {
			'DisableFrontContactStop',
			'DisableFrontCloseStop',
			'DisableRearContactStop',
			'DisableRearCloseStop',
			'MoveForward10',
			}
		})
});

ActivityList[NextActivity] =  'TestMove10'
NextActivity = NextActivity + 1
