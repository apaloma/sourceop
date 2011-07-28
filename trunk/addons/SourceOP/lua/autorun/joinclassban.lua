/**
 * Automatic bans for change class spam based on token buckets.
 */

local jcs_bucket = {}
local jcs_tokens = 20

/**
 * A player has changed class. Decrements the player's tokens and bans if zero
 * is reached.
 */
function jcs_playerchangedclass( pPlayer, newClass )
  local playerIndex = pPlayer:EntIndex()
  local currentTokens = jcs_bucket[playerIndex] or jcs_tokens
  jcs_bucket[playerIndex] = currentTokens - 1
  if(jcs_bucket[playerIndex] <= 0) then
    pPlayer:BanByID(60, "Automatic temp ban for joinclass spam")
    jcs_bucket[playerIndex] = jcs_tokens
  end
end

/**
 * A player has entered the game. Reset the token count.
 */
function jcs_playeractivate( pPlayer )
  jcs_bucket[pPlayer:EntIndex()] = jcs_tokens
end

/**
 * Every second, increment everyone's tokens up to a limit.
 */
function jcs_incrementtokens()
  for k, v in pairs(jcs_bucket) do
    if(v < jcs_tokens) then
      jcs_bucket[k] = jcs_bucket[k] + 2
    end
  end
end

/**
 * Sets up the timers on map init.
 */
function jcs_levelinit()
  // old timers by the same name are automatically removed
  timer.Create("joinclassspaminctokens", 1, 0, jcs_incrementtokens)
end

function jcs_playercommand(pPlayer, command, arguments)
  if(command == "joinclass") then
    jcs_playerchangedclass(pPlayer, arguments[1])
  end
end

//hook.Add("PlayerChangedClass", "joinclassspamchangeclass", jcs_playerchangedclass)
hook.Add("PlayerCommand", "joinclassplayercommand", jcs_playercommand)
hook.Add("PlayerActivate", "joinclassspamactivate", jcs_playeractivate)
hook.Add("LevelInit", "joinclassspamlevelinit", jcs_levelinit)
timer.Create("joinclassspaminctokens", 1, 0, jcs_incrementtokens)
