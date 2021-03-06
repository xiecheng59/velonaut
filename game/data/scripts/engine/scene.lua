require "engine.object"
local gfx = require "engine.graphics.c"
local gfxscene = require "engine.graphics.scene.c"

Scene = class()

function Scene:_init(data)
    self._handle = gfxscene.create()
    gfx.setActiveScene(self._handle)
    self._objects = {}
    self._started = false
    self._remotePlayers = {}
	
    self._deleteCache = {}
    self._controlsInverted = false
end

function Scene:setControlsInverted(b)
    self._controlsInverted = b
end

function Scene:getControlsInverted()
    return self._controlsInverted
end

function Scene:loadPlayers(players, playerId)
	local ringSampleIndex = 0
	local initRadius = 65
	local tangent = Vector(0,0,-1)
	local startingPoint = Vector(0,0,0)
	local normal = Vector(0,1,0)

    for _, player in ipairs(players) do
        local name = "player" .. ringSampleIndex

		local prefab
		if player.id == playerId then
			prefab = "player"
			self.playerName = name
		else
			prefab = "remoteplayer"
		end

		local obj = self:createObject(name)
        local data = loadDataFile(prefab, "object")
		
		obj:load(data)
		obj:start()
		
		local theta = ringSampleIndex * 2 * math.pi / #players
		local ringSample = startingPoint + (normal * (math.cos(theta) * initRadius)) + 
				((tangent:cross(normal):getNormalized()) * (math.sin(theta) * initRadius))
		ringSampleIndex = ringSampleIndex+1	       

        obj:getComponent("RigidBody"):setPosition(ringSample)   
        obj:transform():setPosition(ringSample)   


		if player.id == playerId then
			obj:getComponent("Player"):setId(playerId)
            self._player = obj
		else
			obj:getComponent("RemotePlayer"):setId(player.id)  
            self._remotePlayers[#self._remotePlayers+1] = obj
		end
    end	

    local name = "cameraman"
    local prefab = "cameraman"
    local obj = self:createObject(name)
    local data = loadDataFile(prefab, "object")
    obj:load(data)
    obj:start()

    local cameratarget = self:createObject("cameratarget")
    local targetdata = loadDataFile("cameratarget", "object")
    cameratarget:load(targetdata)
    cameratarget:start()

    cameratarget:transform():setParent(self._player:transform())
    obj:transform():setParent(cameratarget:transform())
    obj:getComponent("CameraMan"):initializePosition()

	self:setMainCamera(self:findObject("cameraman"):getComponent("Camera"))	
end

function Scene:_loadObjectData(data)
    local name = data.name
    local prefab = data.prefab
    assert(name and name ~= "")
    local obj = self:createObject(name)
    local prefabData = loadDataFile(data.prefab, "object")
    obj:load(prefabData)
    if data.children then
        for _, objData in ipairs(data.children) do
            local child = self:_loadObjectData(objData)
            child:transform():setParent(obj:transform())
        end
    end
    return obj
end

function Scene:load(data)
    assert(data.objects)
    for _, objectData in ipairs(data.objects) do
        self:_loadObjectData(objectData)
    end

    local obj = self:findObject(data.mainCamera)
    if obj then 
        local cam = obj:getComponent("Camera")
        assert(cam)
        if cam then self:setMainCamera(cam) end
    end

    local bgColor = data.backgroundColor
    if bgColor then gfx.setBackgroundColor(bgColor) end

    local ambient = data.ambient
    if ambient then gfxscene.setAmbientLight(ambient) end
end

function Scene:start()
    local function onGameMessageReceived(event)

        if event.eventType == "playerUpdate" and event.playerId == self:getId() then        
            self.RigidBody:clearForces()
            
            local curPosition = (Vector(event.position) - self.Transform:position()) / 0.03;

--          self.RigidBody:setPosition(Vector(event.position))
            self.RigidBody:setOrientation(Vector(event.orientation))
            self.RigidBody:setLinearVelocity(curPosition)
            self.RigidBody:setAngularVelocity(Vector(event.angularVelo))
            self.RigidBody:applyCentralForce(Vector(event.force))
            self.RigidBody:applyTorque(Vector(event.torque))
            self._inTun = event.inTun

            return
        end

        if event.eventType == "projectileUpdate" and event.playerId == self:getId() then
            if self._activeProjectiles[event.projectileName] == nil and App.scene():findObject(event.projectileName) == nil then
                local prefab = "projectile"

                local obj = App:scene():createObject(event.projectileName)
                local data = loadDataFile(prefab, "object")
                obj:load(data)
                obj:start()

                local rigidbody = obj:getComponent("RigidBody")
                local transform = obj:transform()
                transform:setPosition(Vector(event.position))
                rigidbody:setPosition(Vector(event.position))
                rigidbody:setLinearVelocity(Vector(event.linearVelo))

                self._activeProjectiles[event.projectileName] = obj
            elseif self._activeProjectiles[event.projectileName] == nil and App.scene():findObject(event.projectileName) ~= nil then
                return
            else
                local obj = self._activeProjectiles[event.projectileName]
                local rigidbody = obj:getComponent("RigidBody")
                local transform = obj:transform()
                rigidbody:setLinearVelocity(Vector(event.linearVelo))
            end
        end
    end

    assert(self._started == false)
    for k, v in pairs(self._objects) do
        v:start()
    end
    self._started = true
end

function Scene:update(dt)
    local removed = {}
    for i = 1, #self._deleteCache do
        table.insert(removed, i)
        self:destroyObject(self._deleteCache[i])
    end
    for i = 1, #removed do table.remove(self._deleteCache, removed[i]) end
    for k, v in pairs(self._objects) do
        v:update(dt)
    end
end

function Scene:player()
    return self._player
end

function Scene:remotePlayers()
    return self._remotePlayers
end

function Scene:setMainCamera(cam)
    gfxscene.setMainCamera(cam._handle)
end

function Scene:createObject(name)
    assert(self._objects[name] == nil)
    local obj = Object(name)
    self._objects[name] = obj
    return obj
end

function Scene:findObject(name)
    return self._objects[name]
end

function Scene:destroyObject(name)
    local obj = self:findObject(name)
    --assert(obj)
    if obj ~= nil then
        obj:onDestroy()
        self._objects[name] = nil
    end
end

function Scene:addToDeleteCache(name)
    table.insert(self._deleteCache, name)
end
