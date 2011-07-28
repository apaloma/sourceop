/** \file cleanup.lua
 *  \brief Executed prior to Lua unloading.
 *
 * @author Tony Paloma
 * @version 12/3/2007
*/

concommand.RemoveAll()
convar.RemoveAll()

Msg("[SOURCEOP] cleanup.lua: " .. _VERSION .. " cleaned up.\n")
