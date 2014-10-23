game_require("textureatlas.lua")

GearStartX = GetConfigF("GearStartX", "")
GearWidth = GetConfigF("GearWidth", "")
GearHeight = GetConfigF("GearHeight", "")
ChannelSpace = "Channels" .. Channels

if Channels == 16 or Channels == 12 then -- DP styles
	GearWidth = GearWidth * 2
end

skin_require("Explosions.lua")
skin_require("ComboDisplay.lua")
skin_require("KeyLightning.lua")
skin_require("FixedObjects.lua")
skin_require("AnimatedObjects.lua")
skin_require("ScoreDisplay.lua")
skin_require("GameplayObjects.lua")

skin_require("TextDisplay.lua")

-- All of these will be loaded in the loading screen instead of
-- in the main thread once loading is over.
Preload = {
	"judgeline.png",
	"filter.png",
	"stage-left.png",
	"stage-right.png",
	"pulse_ver.png",
	"stagefailed.png",
	"progress_tick.png",
	"stage-lifeb.png",
	"stage-lifeb-s.png",
	"combosheet.png",
	"explsheet.png",
	"holdsheet.png",
	"note1.png",
	"note2.png",
	"note3.png",
	"note1L.png",
	"note2L.png",
	"note3L.png",
	"key1.png",
	"key2.png",
	"key3.png",
	"key1d.png",
	"key2d.png",
	"key3d.png",
	"judge-perfect.png",
	"judge-excellent.png",
	"judge-good.png",
	"judge-bad.png",
	"judge-miss.png",
	"auto.png"
}


AnimatedObjects = {

	-- Insert objects in this list.
	List = {
		Filter,
		Pulse,
		JudgeLine,
		StageLines,
		ProgressTick,
		HitLightning,
		ComboDisplay,
		ScoreDisplay,
		Lifebar,
		MissHighlight,
		Explosions,
		Judgment
	},

	-- Internal functions for automating stuff.
	Init = function ()
		for i = 1, #AnimatedObjects.List do
			AnimatedObjects.List[i].Init()
		end
	end,

	Run = function (Delta)
		for i = 1, #AnimatedObjects.List do
			if AnimatedObjects.List[i].Run ~= nil then
				if AnimatedObjects.List[i].Object ~= nil then
					Obj.SetTarget(AnimatedObjects.List[i].Object)
				end

				AnimatedObjects.List[i].Run(Delta)
			end
		end
	end,

	Cleanup = function ()
		for i = 1, #AnimatedObjects.List do
			AnimatedObjects.List[i].Cleanup()
		end
	end
}

BgAlpha = 0

-- You can only call Obj.CreateTarget, LoadImage and LoadSkin during and after Init is called
-- Not on preload time.
function Init()
	AnimatedObjects.Init()
	DrawTextObjects()

	Obj.SetTarget(ScreenBackground)
	Obj.SetAlpha(0)
end

function Cleanup()

	-- When exiting the screen, this function is called.
	-- It's important to clean all targets or memory will be leaked.

	AnimatedObjects.Cleanup()

	if AutoBN then
		Obj.CleanTarget (AutoBN)
	end
end

function getMoveFunction(sX, sY, eX, eY)
	return function(frac)
		Obj.SetPosition(sX + (eX - sX)*frac, sY + (eY - sY)*frac)
		return 1
	end
end

function getUncropFunction(w, h, iw, ih)
	return function(frac)
		Obj.SetSize(w, h*(1-frac))
		Obj.CropByPixels(0, ih*frac*0.5, iw, (ih - (ih*frac*0.5)))
		return 1
	end
end

function OnFullComboEvent()
	fcnotify = Object2D ()
	fcnotify.Image = "fullcombo.png"

	local scalef = GearWidth / fcnotify.Width * 0.85
	fcnotify.X = GearStartX + 2
	fcnotify.ScaleX = scalef
	fcnotify.ScaleY = scalef
	fcanim = getMoveFunction(fcnotify.X, -fcnotify.Height * scalef, fcnotify.X, ScreenWidth/2 - fcnotify.Height*scalef/2)
	Engine:AddTarget(fcnotify)
	Engine:AddAnimation(fcnotify, "fcanim", EaseOut, 0.75, 0)
end

function FailBurst(frac)
	local TargetScaleA = 4
	local TargetScaleB = 3
	local TargetScaleC = 2
	BE.FnA.Alpha = 1 - frac
	BE.FnB.Alpha = 1 - frac
	BE.FnC.Alpha = 1 - frac

	BE.FnA.ScaleY = 1 + (TargetScaleA-1) * frac
	BE.FnB.ScaleY = 1 + (TargetScaleB-1) * frac
	BE.FnC.ScaleY = 1 + (TargetScaleC-1) * frac
	BE.FnA.ScaleX = 1 + (TargetScaleA-1) * frac
	BE.FnB.ScaleX = 1 + (TargetScaleB-1) * frac
	BE.FnC.ScaleX = 1 + (TargetScaleC-1) * frac

	return 1
end

function FailAnim(frac)

	local fnh = FailNotif.Height
	local fnw = FailNotif.Width
	local cosfacadd = 0.75
	local cos = math.cos(frac * 2 * math.pi) * cosfacadd
	local ftype = (1-frac)
	local sc = (cos + cosfacadd/2) * (ftype * ftype) * 1.2 + 1
	FailNotif.ScaleY = sc
	FailNotif.ScaleX = sc
	
	Obj.SetTarget(ScreenBackground)
	Obj.SetAlpha(1 - frac)

	if frac == 1 then -- we're at the end
		BE = {}
		BE.FnA = Object2D()
		BE.FnB = Object2D()
		BE.FnC = Object2D()
		BE.FnA.Image = "stagefailed.png"
		BE.FnB.Image = "stagefailed.png"
		BE.FnC.Image = "stagefailed.png"

		BE.FnA.X = ScreenWidth/2
		BE.FnB.X = ScreenWidth/2 
		BE.FnC.X = ScreenWidth/2
		BE.FnA.Y = ScreenHeight/2
		BE.FnB.Y = ScreenHeight/2
		BE.FnC.Y = ScreenHeight/2
		BE.FnA.Centered = 1
		BE.FnB.Centered = 1
		BE.FnC.Centered = 1
		BE.FnA.Alpha = 0
		BE.FnB.Alpha = 0
		BE.FnB.Alpha = 0
		BE.FnA.Z = 31
		BE.FnB.Z = 31
		BE.FnC.Z = 31

		Engine:AddTarget(BE.FnC)
		Engine:AddTarget(BE.FnB)
		Engine:AddTarget(BE.FnA)
		Engine:AddAnimation(BE.FnA, "FailBurst", EaseOut, 0.7, 0)
	end

	return 1
end

function WhiteFailAnim(frac)
	White.Height = ScreenHeight * frac
	return 1
end

-- Returns duration of failure animation.
function OnFailureEvent()
	
	White = Object2D()
	FailNotif = Object2D()

	White.Centered = 1
	White.X = ScreenWidth / 2
	White.Y = ScreenHeight / 2
	White.Height = 0
	White.Image = "white.png"
	White.Width = ScreenWidth
	FailNotif.Image = "stagefailed.png"
	FailNotif.Centered = 1
	FailNotif.X = ScreenWidth / 2
	FailNotif.Y = ScreenHeight / 2

	White.Z = 30
	FailNotif.Z = 31

	Engine:AddTarget(White)
	Engine:AddTarget(FailNotif)

	Engine:AddAnimation(White, "WhiteFailAnim", EaseIn, 0.35, 0)
	Engine:AddAnimation(FailNotif, "FailAnim", EaseNone, 0.75, 0)

	return 2
end

function BackgroundFadeIn(frac)
	Obj.SetAlpha(frac)
	return 1
end

-- When 'enter' is pressed and the game starts, this function is called.
function OnActivateEvent()

	Obj.SetTarget(ScreenBackground)
	Obj.AddAnimation("BackgroundFadeIn", 1, 0, EaseNone)

	if Auto ~= 0 then
		AutoBN = Obj.CreateTarget()
		
		BnMoveFunction = getMoveFunction(GearStartX + GearWidth/2, -60, GearStartX + GearWidth/2, 100)
			
		Obj.SetTarget(AutoBN)
		Obj.SetImageSkin("auto.png")

		Obj.SetCentered(1)

		Obj.AddAnimation( "BnMoveFunction", 0.75, 0, EaseOut )

		w, h = Obj.GetSize()
		factor = GearWidth / w * 3/4
		Obj.SetSize(w * factor, h * factor)
		Obj.SetZ(28)
		AutoFinishAnimation = getUncropFunction(w*factor, h*factor, w, h)
		RunAutoAnimation = true
	end
end

function HitEvent(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease)
	-- When hits happen, this function is called.
	if math.abs(TimeOff) < AccuracyHitMS then
		DoColor = 0

		if JudgmentValue == 0 then
			DoColor = 1
		end

		Explosions.Hit(Lane, 0, IsHold, IsHoldRelease)
		ComboDisplay.Hit(DoColor)

		local EarlyOrLate
		if TimeOff < 0 then
			EarlyOrLate = 1
		else	
			EarlyOrLate = 2
		end

		Judgment.Hit(JudgmentValue, EarlyOrLate)
	end

	ScoreDisplay.Update()
end

function MissEvent(TimeOff, Lane, IsHold)
	-- When misses happen, this function is called.
	if math.abs(TimeOff) <= 135 then -- mishit
		Explosions.Hit(Lane, 1, IsHold, 0)
	end

	local EarlyOrLate
	if TimeOff < 0 then
		EarlyOrLate = 1
	else
		EarlyOrLate = 2
	end

	Judgment.Hit(5, EarlyOrLate)

	ScoreDisplay.Update()
	ComboDisplay.Miss()
	MissHighlight.OnMiss(Lane)
end

function KeyEvent(Key, Code, IsMouseInput)
	-- All key events, related or not to gear are handled here
end

function GearKeyEvent (Lane, IsKeyDown)
	-- Only lane presses/releases are handled here.

	if Lane >= Channels then
		return
	end

	HitLightning.LanePress(Lane, IsKeyDown)
end

-- Called when the song is over.
function SongFinishedEvent()
	if AutoBN then
		Obj.SetTarget(AutoBN)
		Obj.AddAnimation ("AutoFinishAnimation", 0.35, 0, EaseOut)
		RunAutoAnimation = false
	end
end

function Update(Delta)
	-- Executed every frame.
	
	if Active ~= 0 then
		if AutoBN and RunAutoAnimation == true then
			local BeatRate = Beat / 2
			local Scale = math.sin( math.pi * 2 * BeatRate  )
			Scale = Scale * Scale * 0.25 + 0.75
			Obj.SetTarget(AutoBN)
			Obj.SetScale(Scale, Scale)
		end

	end
	
	AnimatedObjects.Run(Delta)
	UpdateTextObjects()

end

