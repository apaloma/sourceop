/** \file init.lua
 *  \brief Executed prior to the autorun directory.
 *
 * @author Tony Paloma
 * @version 1/18/2008
*/

include("util.lua")
include("class.lua")
include("menu.lua")
include("vote.lua")
include("abstainvote.lua")
include("playerpicker.lua")
include("optionpicker.lua")

include("extensions/player.lua")

require "list"
require "hook"
require "playercommand"
require "admincommand"
require "timer"

// uncomment to get a dump of functions
//include("functiondump.lua")

math.randomseed(os.time())

Msg("[SOURCEOP] init.lua: " .. _VERSION .. " initialized.\n")
