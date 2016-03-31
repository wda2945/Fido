local _PACKAGE = (...):match("^(.+)[%./][^%./]+"):gsub("[%./]?node_types", "")
local class = require(_PACKAGE..'/middleclass')
local BranchNode  = require(_PACKAGE..'/node_types/branch_node')
local RepeatWhileFail = class('RepeatWhileFail', BranchNode)

-- like a Sequence, but repeats the Sequence until a complete success

function RepeatWhileFail:success()
  if (self.name) then
  	Print('RepeatWhileFail ' .. self.name .. ' success @ ' .. self.actualTask .. ' of ' .. #self.nodes)
  else
    Print('RepeatWhileFail: success @ ' .. self.actualTask .. ' of ' .. #self.nodes)
  end

  BranchNode.success(self)
  self.actualTask = self.actualTask + 1
  if self.actualTask <= #self.nodes then
    self:_run(self.object)
  else
    self.control:success()
  end
end

function RepeatWhileFail:fail()
  if (self.name) then
  	Print('RepeatWhileFail ' .. self.name .. ' fail @ ' .. self.actualTask .. ' of ' .. #self.nodes)
  else
    Print('RepeatWhileFail: fail @ ' .. self.actualTask .. ' of ' .. #self.nodes)
  end
  BranchNode.fail(self)
  self.actualTask = 1
  self:_run(self.object)
end

return RepeatWhileFail
