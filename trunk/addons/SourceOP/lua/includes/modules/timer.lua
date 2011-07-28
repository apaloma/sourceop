/**
 * \file timer.lua
 * \brief A module that allows creating timers that execute functions at
 *        specified intervals.
 *
 * Thanks to Team Garry. Almost all of this module comes from Garry's Mod.
 */


local hook = require "hook"

// Globals that we need.
local CurTime = game.CurTime
local UnPredictedCurTime = game.CurTime
local unpack = unpack
local pairs = pairs
local table = table
local pcall = pcall
local Error = Error
local tostring = tostring

module("timer")

// Some definitions
local PAUSED = -1
local STOPPED = 0
local RUNNING = 1

// Declare our locals
local Timer = {}
local TimerSimple = {}

local function CreateTimer( name )
  if ( Timer[name] == nil ) then
    Timer[name] = {}
    Timer[name].Status = STOPPED
    return true
  end
  return false
end

/*---------------------------------------------------------
   Name: IsTimer( name )
   Desc: Returns boolean whether or not name is a timer.
---------------------------------------------------------*/
function IsTimer( name )
  if ( Timer[name] == nil ) then return false
  else return true end
end

/*---------------------------------------------------------
   Name: Create( name, delay, reps, func, ... )
   Desc: Setup and start a timer by name.
---------------------------------------------------------*/
function Create( name, delay, reps, func, ... )
  if ( IsTimer( name ) ) then
    Destroy( name )
  end
  Adjust( name, delay, reps, func, unpack(arg) )
  Start( name )
end

/*---------------------------------------------------------
   Name: Start( name )
   Desc: (Re)start the timer by name.
---------------------------------------------------------*/
function Start( name )
  if ( !IsTimer( name ) ) then return false; end
  Timer[name].n = 0
  Timer[name].Status = RUNNING
  Timer[name].Last = CurTime()
  return true;
end

/*---------------------------------------------------------
   Name: Adjust( name, delay, reps, func, ... )
   Desc: Adjust a running, stopped or paused timer by name.
---------------------------------------------------------*/
function Adjust( name, delay, reps, func, ... )
  CreateTimer( name )
  Timer[name].Delay = delay
  Timer[name].Repetitions = reps
  if ( func != nil ) then Timer[name].Func = func end
  if ( arg != nil ) then Timer[name].Args = arg end
  return true;
end

/*---------------------------------------------------------
   Name: Pause( name )
   Desc: Pause a running timer by name.
---------------------------------------------------------*/
function Pause( name )
  if ( !IsTimer( name ) ) then return false; end
  if ( Timer[name].Status == RUNNING ) then
    Timer[name].Diff = CurTime() - Timer[name].Last
    Timer[name].Status = PAUSED
    return true
  end
  return false
end

/*---------------------------------------------------------
   Name: UnPause( name )
   Desc: Unpause a paused timer by name.
---------------------------------------------------------*/
function UnPause( name )
  if ( !IsTimer( name ) ) then return false; end
  if ( Timer[name].Status == PAUSED ) then
    Timer[name].Diff = nil
    Timer[name].Status = RUNNING
    return true
  end
  return false
end

/*---------------------------------------------------------
   Name: Toggle( name )
   Desc: Toggle a timer's pause state by name.
---------------------------------------------------------*/
function Toggle( name )
  if ( IsTimer( name ) ) then
    if ( Timer[name].Status == PAUSED ) then
      return UnPause( name )
    elseif ( Timer[name].Status == RUNNING ) then
      return Pause( name )
    end
  end
  return false
end

/*---------------------------------------------------------
   Name: Stop( name )
   Desc: Stop a running or paused timer by name.
---------------------------------------------------------*/
function Stop( name )
  if ( !IsTimer( name ) ) then return false; end
  if ( Timer[name].Status != STOPPED ) then
    Timer[name].Status = STOPPED
    return true
  end
  return false
end

/*---------------------------------------------------------
   Name: Check()
   Desc: Check all timers and complete any tasks needed.
    This should be run every frame.
---------------------------------------------------------*/
function Check()

  local curTime = CurTime()

  for key, value in pairs( Timer ) do

    if ( value.Status == PAUSED ) then

      value.Last = curTime - value.Diff

    elseif ( value.Status == RUNNING && ( value.Last + value.Delay ) <= curTime ) then

      value.Last = curTime
      value.n = value.n + 1

      local b, e = pcall( value.Func, unpack( value.Args ) )
      if ( !b ) then
        Error("Timer Error: "..tostring(e).."\n")
      end

      if ( value.n >= value.Repetitions && value.Repetitions != 0) then
        Stop( key )
      end

    end

  end

  // Run Simple timers
  for key, value in pairs( TimerSimple ) do

    if ( value.Finish <= curTime ) then

      local b, e = pcall( value.Func, unpack( value.Args ) )
      if ( !b ) then
        Error("Timer Error: "..tostring(e).."\n")
      end

      TimerSimple[ key ] = nil      // Kill Timer

    end
  end

end

/*---------------------------------------------------------
   Name: Destroy( name )
   Desc: Destroy the timer by name and remove all evidence.
---------------------------------------------------------*/
function Destroy( name )
  Timer[name] = nil
end

Remove = Destroy

/*---------------------------------------------------------
   Name: Simple( delay, func, ... )
   Desc: Make a simple "create and forget" timer
---------------------------------------------------------*/
function Simple( delay, func, ... )

  local new_timer = {}

  new_timer.Finish = UnPredictedCurTime() + delay

  if ( func != nil ) then new_timer.Func = func end
  if ( arg != nil ) then new_timer.Args = arg end

  table.insert( TimerSimple, new_timer )

  return true;

end

hook.Add( "Think", "CheckTimers", Check )
