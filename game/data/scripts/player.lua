require "engine.scene"
require "engine.vector"
require "engine.behavior"
require "engine.input"
require "engine.rigidbody"
require "engine.camera"

local Network = require "engine.network.c"

Player = class(Behavior)

function Player:start()
	self.RigidBody = self:getComponent("RigidBody")
	self.Transform = self:getComponent("Transform")

	self._active = false

	-- Create first checkpoint 
	self._nextCheckpoint = 1
	self._inventory = {}
	self:createNextCheckpoint()
	self:createPickups()

	self._boostSpeed = 900
	self._boostCapacity = 5
	self._boostFuel = 0

	self._activeProjectiles = {}
	self._projectileSpeed = 1200
	self._projectileCounter = 0
	self._projectileRange = 10000
	self._projectileLife = 30

   if App.scene():getControlsInverted() then
        self._key_down = "key_up"
        self._key_up = "key_down"
    else
        self._key_down = "key_down"
        self._key_up = "key_up"
    end

	self._inTun = true

	local function onGameMessageReceived(event)

		if event.eventType == "gamestart" then	
			if not self._active then self._active = true end
		end	

		if event.eventType == "gameover" then
			self._active = false
		end
	end

	Network.addEventListener("game_message", onGameMessageReceived)

end

function Player:createPickups()
	local tun = App.scene():findObject("tunnel"):getComponent("Tunnel")	
	for index = 1, tun:getNumPickups() do
		local name = "pickup"..index
		local prefab = "pickup"

		local obj = App:scene():createObject(name)
		local data = loadDataFile(prefab, "object")
		obj:load(data)
		obj:start()

		local startPos = tun:getPickupPosition(index)
		obj:transform():setPosition(startPos)
		obj:getComponent("RigidBody"):setPosition(startPos)
	end

end

function Player:createNextCheckpoint()
	local tun = App.scene():findObject("tunnel"):getComponent("Tunnel")	
	if self._nextCheckpoint <= tun:getNumCheckpoints() then
		local name = "checkpoint" .. self._nextCheckpoint
		local prefab = "checkpoint"

		local obj = App.scene():createObject(name)
		local data = loadDataFile(prefab, "object")	
		obj:load(data)
		obj:start()

		local startPos = tun:getCheckpointPosition(self._nextCheckpoint)
		local startTan = tun:getCheckpointTangent(self._nextCheckpoint):getNormalized()

		local angle = Vector(0,0,-1):angleBetween(startTan)
		local axis = (Vector(0,0,-1):cross(startTan)):getNormalized()
		local startOri = Vector(0, 0, 0, 0)
		startOri:makeQuaternionFromAngleAxis(angle, axis)

		obj:transform():setPosition(startPos)
		obj:transform():setOrientation(startOri)
		obj:getComponent("RigidBody"):setPosition(startPos)		
		obj:getComponent("RigidBody"):setOrientation(startOri)

		self._nextCheckpoint = self._nextCheckpoint + 1
	elseif self._nextCheckpoint == tun:getNumCheckpoints()+1 then
		local name = "finishline"
		local prefab = "finishline"

		local obj = App.scene():createObject(name)
		local data = loadDataFile(prefab, "object")	
		obj:load(data)
		obj:start()

		local startPos = tun:getCheckpointPosition(1)
		local startTan = tun:getCheckpointTangent(1):getNormalized()

		local angle = Vector(0,0,-1):angleBetween(startTan)
		local axis = (Vector(0,0,-1):cross(startTan)):getNormalized()
		local startOri = Vector(0, 0, 0, 0)
		startOri:makeQuaternionFromAngleAxis(angle, axis)

		obj:transform():setPosition(startPos)
		obj:transform():setOrientation(startOri)
		obj:getComponent("RigidBody"):setPosition(startPos)		
		obj:getComponent("RigidBody"):setOrientation(startOri)
	end
end

function Player:setId( id )
	self._id = id
end

function Player:getId()
	return self._id
end

function Player:update(dt)

	if not self._active then
		return
	end

	--local cammantrans = App.scene():findObject("cameraman"):getComponent("Transform")
	--local thrust = self:transform():localZ() * -1
	local look = self:transform():localZ() * -1
	local up = self:transform():localY()
	local right = self:transform():localX() * -1


	local rotScale = 200
	local linScale = 800
	
	self._inTun = true
	local tun = App.scene():findObject("tunnel"):getComponent("Tunnel")
	local tunnelDist = tun:getClosestSamplePosition(self.Transform:position())
	if tunnelDist.distance > (tun:tunnelRadius() * 1.1) then 
		self._inTun = false
	end

	if self._active then
		if Input.getKey(self._key_up) then self.RigidBody:applyTorque(right * -rotScale) end
		if Input.getKey(self._key_down) then self.RigidBody:applyTorque(right * rotScale) end
		if Input.getKey("key_left") then self.RigidBody:applyTorque(up * rotScale) end
		if Input.getKey("key_right") then self.RigidBody:applyTorque(up * -rotScale) end
		if Input.getKey("key_a") then self.RigidBody:applyTorque(look * -rotScale) end
		if Input.getKey("key_d") then self.RigidBody:applyTorque(look * rotScale) end
		if Input.getKey("key_space") then 
			if self._inTun then self.RigidBody:applyCentralForce(look * linScale) 
			else self.RigidBody:applyCentralForce(look * linScale * 0.5) end
		end
		if Input.getKeyDown("key_lshift") and #self._inventory > 0 then
			self:useItem(self._inventory[1][1])
			self._inventory[1][2] = self._inventory[1][2] - 1
			if (self._inventory[1][2] < 1) then self._inventory = {} end
		end
		if Input.getKeyDown("key_f") then
			self:useItem("projectile")
		end
	end

	self:updateItems()
	
	local hud = App.scene():findObject("hud"):getComponent("Hud")
	if #self._inventory > 0 then
		hud:setPickupName( self._inventory[1][1] )
		hud:setPickupNumber( self._inventory[1][2] )
	else
		hud:setNoPickup()
	end		

	self:sendPhysics()
end

function Player:onCollision(collision)
    if collision.rigidbody:owner():getComponent("Checkpoint") ~= nil then
		local checkpoint = collision.rigidbody:owner()
    	checkpoint:destroy()
		self:createNextCheckpoint()
		App.scene():findObject("hud"):getComponent("Hud"):incrementCheckpoints()

	elseif collision.rigidbody:owner():getComponent("Pickup") ~= nil then
		local pickup = collision.rigidbody:owner():getComponent("Pickup"):item()
		if #self._inventory == 0 then
			self._inventory[1] = pickup
		elseif self._inventory[1][1] == pickup[1] then
			self._inventory[1][2] = self._inventory[1][2] + pickup[2]
		end
		collision.rigidbody:owner():destroy()
	elseif collision.rigidbody:owner():getComponent("FinishLine") ~= nil then
		Network.RPC("setGameOver", "")
	end
end

function Player:sendPhysics()
	local position 		= self.RigidBody:position()
	local orientation 	= self.RigidBody:orientation()
	local linearVelo	= self.RigidBody:linearVelocity()
	local angularVelo	= self.RigidBody:angularVelocity()
	local force			= self.RigidBody:force()
	local torque		= self.RigidBody:torque()

	Network.sendMessage( "{ eventType='playerUpdate'" ..
						 ",playerId='" .. self:getId() .. "'" ..
						 ",position=" .. position:serialize3d() ..
						 ",orientation=" .. orientation:serialize4d() ..
   						 ",linearVelo=" .. linearVelo:serialize3d() ..
						 ",angularVelo=" .. angularVelo:serialize3d() ..
 						 ",force=" .. force:serialize3d() ..
						 ",torque=" .. torque:serialize3d() .. 
						 ",inTun=" .. tostring(self._inTun) .. "}")

end

function Player:updateItems()
	if self._boostFuel > 0 then
		local rigidbody = self:owner():getComponent("RigidBody")
		local transform = self:owner():getComponent("Transform")
		local look = transform:orientation():applyRotationTo(Vector(0,0,-1))
		rigidbody:setLinearVelocity(look:getNormalized() * self._boostSpeed)
		self._boostFuel = self._boostFuel - 1
	end

	local remotePlayers = App.scene():remotePlayers()

	local remove = {}
	for k, v in pairs(self._activeProjectiles) do
		local target = 0
		local targetDist = math.huge
		local pos = v[1]:transform():position()

        if (self:owner():transform():position() - pos):length() > self._projectileRange then
        	table.insert(remove, k)
        else
			if v[2] > 0 then
				v[2] = v[2] - 1

				for i = 1, #remotePlayers do
					local relativePos = pos - remotePlayers[i]:transform():position()
					if relativePos:length() < targetDist then
						targetDist = relativePos:length()
						target = i
					end
				end

				if target ~= 0 then
					if v[2] / self._projectileLife < 0.85 then
						local tar = (remotePlayers[target]:transform():position() - pos):getNormalized()
						local vel = (v[1]:getComponent("RigidBody"):linearVelocity()):getNormalized()
						local ratio = v[2] / self._projectileLife
						tar = tar * (1-ratio)
						vel = vel * ratio
						local newVel = ((tar + vel)/2):getNormalized() * self._projectileSpeed 
						v[1]:getComponent("RigidBody"):setLinearVelocity(newVel)
					end
				end

			end

			self:sendProjectile(v[1])
		end
    end

    for i = 1, #remove do
    	self._activeProjectiles[remove[i]] = nil
	end
end

function Player:sendProjectile(projectile)
	local rigidbody = projectile:getComponent("RigidBody") 
	local position 		= rigidbody:position()
	local linearVelo	= rigidbody:linearVelocity()

	Network.sendMessage( "{ eventType='projectileUpdate'" ..
						 ",projectileName='" .. projectile:name() .. "'" ..
						 ",playerId='" .. self:getId() .. "'" ..
						 ",position=" .. position:serialize3d() ..
   						 ",linearVelo=" .. linearVelo:serialize3d() ..  "}")

end


function Player:useItem(item)

	if item == "boost" then
		self._boostFuel = self._boostCapacity
	end

	if item == "projectile" then

		local rigidbody = self:owner():getComponent("RigidBody")
		local transform = self:owner():getComponent("Transform")
		local look = transform:orientation():applyRotationTo(Vector(0,0,-1)):getNormalized()

		self._projectileCounter = self._projectileCounter + 1
		local name = "projectile_"..self:owner():name().."_".. self._projectileCounter
		local prefab = "projectile"

		local obj = App:scene():createObject(name)
		local data = loadDataFile(prefab, "object")
		obj:load(data)
		obj:start()

		obj:getComponent("RigidBody"):setTrigger(true)
		local startPos = rigidbody:position() + (look * 30)
		obj:transform():setPosition(startPos)
		obj:getComponent("RigidBody"):setPosition(startPos)
		obj:getComponent("RigidBody"):setLinearVelocity(look * self._projectileSpeed)
		obj:getComponent("Projectile"):setSender(self:owner():name())

		self._activeProjectiles[name] = {obj, self._projectileLife}
	end

end

function Player:destroyProjectile(name)
	local obj = App.scene():findObject(name)
	if self._activeProjectiles[name] ~= nil then
		self._activeProjectiles[name] = nil
		obj:destroy()
	end
end
