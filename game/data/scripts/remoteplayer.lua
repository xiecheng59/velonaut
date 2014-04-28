require "engine.vector"
require "engine.behavior"
require "engine.input"
require "engine.rigidbody"
require "engine.camera"

local Network = require "engine.network.c"

RemotePlayer = class(Behavior)

function RemotePlayer:start()
	self.RigidBody = self:getComponent("RigidBody")
	self.Transform = self:getComponent("Transform")
	self._activeProjectiles = {}
	
	local function onGameMessageReceived(event)

		if event.eventType == "playerUpdate" and event.playerId == self:getId() then		
			self.RigidBody:clearForces()
			
			local curPosition = (Vector(event.position) - self.Transform:position()) / 0.03;

--			self.RigidBody:setPosition(Vector(event.position))
			self.RigidBody:setOrientation(Vector(event.orientation))
			self.RigidBody:setLinearVelocity(curPosition)
			self.RigidBody:setAngularVelocity(Vector(event.angularVelo))
			self.RigidBody:applyCentralForce(Vector(event.force))
			self.RigidBody:applyTorque(Vector(event.torque))

			return
		end

		if event.eventType == "projectileUpdate" and event.playerId == self:getId() then
			if self._activeProjectiles[event.projectileName] == nil then
				local prefab = "projectile"

				local obj = App:scene():createObject(event.projectileName)
				local data = loadDataFile(prefab, "object")
				obj:load(data)
				obj:start()

				local rigidbody = obj:getComponent("RigidBody")
				local transform = obj:transform()
				transform:setPosition(Vector(event.position))
				transform:setOrientation(Vector(event.orientation))
				rigidbody:setPosition(Vector(event.position))
				rigidbody:setOrientation(Vector(event.orientation))
				rigidbody:setLinearVelocity(Vector(event.linearVelo))
				rigidbody:setAngularVelocity(Vector(event.angularVelo))
				rigidbody:applyCentralForce(Vector(event.force))
				rigidbody:applyTorque(Vector(event.torque))

				self._activeProjectiles[event.projectileName] = obj
			else
				local obj = self._activeProjectiles[event.projectileName]
				local rigidbody = obj:getComponent("RigidBody")
				local transform = obj:transform()
				transform:setOrientation(Vector(event.orientation))
				rigidbody:setOrientation(Vector(event.orientation))
				rigidbody:setLinearVelocity(Vector(event.linearVelo))
				rigidbody:setAngularVelocity(Vector(event.angularVelo))
				rigidbody:applyCentralForce(Vector(event.force))
				rigidbody:applyTorque(Vector(event.torque))
			end
		end
	end

	Network.addEventListener("game_message", onGameMessageReceived)
end

function RemotePlayer:setId( id )
	self._id = id
end

function RemotePlayer:getId()
	return self._id
end

function RemotePlayer:update(dt)
	
	

end

