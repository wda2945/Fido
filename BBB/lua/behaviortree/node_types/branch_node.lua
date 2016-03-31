local _PACKAGE = (...):match("^(.+)[%./][^%./]+"):gsub("[%./]?node_types", "")
local class = require(_PACKAGE..'/middleclass')
local Registry = require(_PACKAGE..'/registry')
local Node  = require(_PACKAGE..'/node_types/node')
local BranchNode = class('BranchNode', Node)

function BranchNode:start(object)
  if not self.nodeRunning then
    self:setObject(object)
    self.actualTask = 1
  end
end

function BranchNode:run(object)
  if self.actualTask <= #self.nodes then
    self:_run(object)
  end
end

function BranchNode:_run(object)
  if self.actualTask <= #self.nodes then
  	self.node = Registry.getNode(self.nodes[self.actualTask]) 
  	
  	if not self.node then
--[[  		if self.name then
  			Print('BranchNode ' .. self.name .. ' no node '.. self.actualTask .. ' - ' .. self.nodes[self.actualTask])
  		else
  			Print('BranchNode: no node '.. self.actualTask .. ' - ' .. self.nodes[self.actualTask])  		
  		end
]]
  	else
--[[
  		if self.name then
  			if self.node.name then
  				Print('BranchNode ' .. self.name .. ' calling node '.. self.actualTask .. ' - ' .. self.node.name)
  			else
   				Print('BranchNode ' .. self.name .. ' calling node '.. self.actualTask) 				
  			end
  		else
  			if self.node.name then
  				Print('BranchNode calling node '.. self.actualTask .. ' - ' .. self.node.name)
  			else
   				Print('BranchNode calling node '.. self.actualTask) 				
  			end		
  		end
]]
		if not self.nodeRunning then
		  self.node:start(object)
		end
		self.node:setControl(self)
		self.node:call_run(object)
    end
  end
end

function BranchNode:running()
  self.nodeRunning = true
  self.control:running()
end

function BranchNode:success()
  self.nodeRunning = false
  self.node:finish(self.object)
  self.node = nil
end

function BranchNode:fail()
  self.nodeRunning = false
  self.node:finish(self.object);
  self.node = nil
end

return BranchNode
