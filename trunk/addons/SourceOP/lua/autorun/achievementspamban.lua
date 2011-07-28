/**
 * Automatic bans for change class spam based on token buckets.
 */

local aes_bucket = {}
local aes_tokens = 38

/**
 * A player has changed class. Decrements the player's tokens and bans if zero
 * is reached.
 */
function aes_decrementtokens( pPlayer )
  local playerIndex = pPlayer:EntIndex()
  aes_bucket[playerIndex] = aes_bucket[playerIndex] - 1
  if(aes_bucket[playerIndex] <= 0) then
    pPlayer:BanByID(0, "Automatic ban for achievement spam")
    aes_bucket[playerIndex] = aes_tokens
  end
end

function aes_playercommand( pPlayer, command, arguments )
  if(command == "achievement_earned") then
    aes_decrementtokens(pPlayer)
  end
end

/**
 * A player has entered the game. Reset the token count.
 */
function aes_playeractivate( pPlayer )
  aes_bucket[pPlayer:EntIndex()] = aes_tokens
end

/**
 * Every second, increment everyone's tokens up to a limit.
 */
function aes_incrementtokens()
  for k, v in pairs(aes_bucket) do
    if(v < aes_tokens) then
      aes_bucket[k] = aes_bucket[k] + 1
    end
  end
end

/**
 * Sets up the timers on map init.
 */
function aes_levelinit()
  // old timers by the same name are automatically removed
  timer.Create("aesinctokens", 1, 0, aes_incrementtokens)
end

hook.Add("PlayerCommand", "aescommand", aes_playercommand)
hook.Add("PlayerActivate", "aesactivate", aes_playeractivate)
hook.Add("LevelInit", "aeslevelinit", aes_levelinit)
