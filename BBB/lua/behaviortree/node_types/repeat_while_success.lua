local _PACKAGE = (...):match("^(.+)[%./][^%./]+"):gsub("[%./]?node_types", "")
local class = require(_PACKAGE..'/middleclass')
local BranchNode  = require(_PACKAGE..'/node_types/branch_node')
local RepeatWhileSuccess = class('RepeatWhileSuccess', BranchNode)

-- like a Sequence, but repeats the Sequence until a fail

function RepeatWhileSuccess:success()
  BranchNode.success(self)
  self.actualTask = self.actualTask + 1
  if self.actualTask <= #self.nodes then
    self:_run(self.object)
  else
    self.actualTask = 1
  	self:_run(self.object)
  end
end

function RepeatWhileSuccess:fail()
  BranchNode.fail(self)
  self.control:fail()
end

return RepeatWhileSuccess
