require "engine.bezier"
require "engine.behavior"
require "engine.mesh"
require "engine.meshbuilder"
require "engine.meshrenderer"

local function getRandomPoint(min, max)
	local x = math.random()-0.5
	local y = math.random()-0.5
	local z = math.random()-0.5
	local dir = Vector(x, y, z)	
	dir = dir:getNormalized()
	local radius = min + (math.random() * (max- min))
	return dir * radius	
end

local function getRandomRadius(min, max)
	return min + (math.random() * (max - min))
end

Tunnel = class(Behavior)

function Tunnel:_init(object)

	-- Params
	local numCurves = 4
	local tunnelRadius = 100
	local ringsPerCurve = 80
	local samplesPerRing = 30
	local minStartRadius = 100
	local maxStartRadius = 150
	local minControlRadius = 6000
	local maxControlRadius = 8000
	local minAnchorRadius = 1000
	local maxAnchorRadius = 1050

	--TODO: set seed globally somewhere else
	local tunnelSeed = 666

	-- Set seed and create table of curves
	math.randomseed(tunnelSeed)
	local curves = {}
	
	-- CREATE THE SPLINE --------------------------------------------------------------------------

	-- First curve
	local p0 = Vector(0, 0, 0)
	local p1 = Vector(0, 0, -1) * getRandomRadius(minStartRadius, maxStartRadius)
	local p2 = p1 + (Vector(0, 0, -1) * getRandomRadius(minStartRadius, maxStartRadius))
	local p3 = p2 + Vector(0, 0, -1) * getRandomRadius(minStartRadius, maxStartRadius)
	curves[1] = Bezier(p0, p1, p2, p3)
	--[[
	print(1 .. " p0: [" .. p0.x .. " " .. p0.y .. " " .. p0.z .. "]"
			.. " p1: [" .. p1.x .. " " .. p1.y .. " " .. p1.z .. "]"
			.. " p2: [" .. p2.x .. " " .. p2.y .. " " .. p2.z .. "]"
			.. " p3: [" .. p3.x .. " " .. p3.y .. " " .. p3.z .. "]")
	--]]
	-- Middle of spline
	for i = 2, numCurves-1 do
		p0 = curves[i-1].p3
		p1 = p0 + (((p0 - curves[i-1].p2):getNormalized()) * getRandomRadius(minControlRadius, maxControlRadius))
		p3 = p0 + getRandomPoint(minAnchorRadius, maxAnchorRadius)
		p2 = p3 + getRandomPoint(minControlRadius, maxControlRadius)
		--[[
		print(i .. " p0: [" .. p0.x .. " " .. p0.y .. " " .. p0.z .. "]"
			  	.. " p1: [" .. p1.x .. " " .. p1.y .. " " .. p1.z .. "]"
				.. " p2: [" .. p2.x .. " " .. p2.y .. " " .. p2.z .. "]"
				.. " p3: [" .. p3.x .. " " .. p3.y .. " " .. p3.z .. "]")
		--]]
		curves[i] = Bezier(p0, p1, p2, p3)
	end

	-- Last curve
	p0 = curves[numCurves-1].p3
	p1 = p0 + (((p0 - curves[numCurves-1].p2):getNormalized()) * ((p3-p1):length()+minAnchorRadius))
	p3 = curves[1].p0
	p2 = p3 + (((p3 - curves[1].p1):getNormalized()) * ((p3-p1):length()+minAnchorRadius))
	--[[
	print(numCurves .. " p0: [" .. p0.x .. " " .. p0.y .. " " .. p0.z .. "]"
					.. " p1: [" .. p1.x .. " " .. p1.y .. " " .. p1.z .. "]"
					.. " p2: [" .. p2.x .. " " .. p2.y .. " " .. p2.z .. "]"
					.. " p3: [" .. p3.x .. " " .. p3.y .. " " .. p3.z .. "]")
	--]]
	curves[numCurves] = Bezier(p0, p1, p2, p3)

	-- CREATE THE SAMPLES -------------------------------------------------------------------------

	local normalHelper = Vector(0,1,0)
	local lastNormal = Vector(0,1,0)
	local ringSamples = {}
	local curveSamples = {}

	for curveIndex = 1, numCurves do
		local bezier = curves[curveIndex]
		for t = 0, 1-(1/ringsPerCurve), 1/ringsPerCurve do
			local point = bezier:getPoint(t)
			local tangent = (bezier:getDerivativePoint(t)):getNormalized()
			local normal = (tangent:cross(normalHelper)):getNormalized()
			if normal.x < math.epsilon and normal.x > (-math.epsilon) and 
			   normal.y < math.epsilon and normal.y > (-math.epsilon) and 
			   normal.z < math.epsilon and normal.z > (-math.epsilon) then normal = lastNormal end

			normalHelper = normal:cross(tangent)
			lastNormal = normal
			curveSamples[#curveSamples+1] = point

			for ringSampleIndex = 0, samplesPerRing-1 do
				local theta = ringSampleIndex * 2 * math.pi / samplesPerRing
				local ringSample = point + (normal * (math.cos(theta) * tunnelRadius)) + 
						((tangent:cross(normal):getNormalized()) * (math.sin(theta) * tunnelRadius))
				ringSamples[#ringSamples+1] = ringSample
			end
		end
	end

	-- CREATE THE MESH ----------------------------------------------------------------------------

	local numRings = numCurves * ringsPerCurve
	local mb = MeshBuilder("LineStrip")
	--print("NUMRINGS " ..numRings)
	for ringIndex = 0, numRings-1 do
		--print("RING "..ringIndex)
		for ringSampleIndex = 0, samplesPerRing-1 do
			local ind = ((ringIndex * samplesPerRing) + ringSampleIndex) + 1 
			local ringSample = ringSamples [ ((ringIndex * samplesPerRing) + ringSampleIndex) + 1 ]
			local sampleNormal = ringSample-curveSamples[ringIndex + 1]			
			mb:position(ringSample)
			--print("ring sample "..ind..": " ..ringSample.x.." "..ringSample.y.." "..ringSample.z)
			mb:normal(sampleNormal)			
		end
	end
	for ringIndex =  0, numRings-1 do
		local currRingSampleBase = ringIndex * samplesPerRing
		local nextRingSampleBase = (currRingSampleBase + samplesPerRing) % #ringSamples
		for ringSampleIndex = 0, samplesPerRing-2 do
			mb:index(nextRingSampleBase+ringSampleIndex+1)		
			mb:index(currRingSampleBase+ringSampleIndex+1)
		end
	end

	--print("samples: " .. #ringSamples)

	local m = mb:getMesh()
	local mr = object:addComponent("MeshRenderer", nil, false)
	mr:setMesh(m)

--[[
    local mb = MeshBuilder("LineStrip") -- or "LineStrip"
    mb:position(Vector(0, 0, 0))
    mb:normal(Vector(0, 0, 1))
    mb:position(Vector(1, 0, 0))
    mb:normal(Vector(0, 0, 1))
    mb:position(Vector(0, 1, 0))
    mb:normal(Vector(0, 0, 1))
    mb:index(0)
    mb:index(1)
    mb:index(2)
    mb:index(0)
    local m = mb:getMesh()

    local mr = object:addComponent("MeshRenderer", nil, false)
    mr:setMesh(m)
--]]
end

function Tunnel:start()

end

function Tunnel:update(dt)

end


