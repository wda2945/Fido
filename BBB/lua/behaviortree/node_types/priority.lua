local _PACKAGE = (...):match("^(.+)[%./][^%./]+"):gsub("[%./]?node_types", "")
local class = require(_PACKAGE..'/middleclass')
local BranchNode  = require(_PACKAGE..'/node_types/branch_node')
local Priority = class('Priority', BranchNode)

function Priority:success()
  if (self.name) then
  	Print('Priority ' .. self.name .. ' success @ ' .. self.actualTask .. ' of ' .. #self.nodes)
  else
    Print('Priority: success @ ' .. self.actualTask .. ' of ' .. #self.nodes)
  end

  BranchNode.success(self)
  self.control:success()
end

function Priority:fail()
  if (self.name) then
  	Print('Priority ' .. self.name .. ' fail @ ' .. self.actualTask .. ' of ' .. #self.nodes)
  else
    Print('Priority: fail @ ' .. self.actualTask .. ' of ' .. #self.nodes)
  end

  BranchNode.fail(self)
  self.actualTask = self.actualTask + 1
  if self.actualTask <= #self.nodes then
    self:_run(self.object)
  else
    self.control:fail()
  end
end

return Priority
