/**
 * \file list.lua
 * \brief A module that provides basic list modification.
 */

local table = table

module("list")

local lists = {}

/**
 * Gets a list from the dictionary of lists.
 *
 * @param list The list to get.
 * @return The list, or a new one if it does not yet exist.
 */
function Get(list)
  // create the list if it does not yet exist
  lists[list] = lists[list] or {}

  // return the list
  return lists[list]
end

/**
 * Sets a key/value pair in a list.
 *
 * @param list The list to set the pair on.
 * @param key The key to set.
 * @param value The value to set.
 */
function Set(list, key, value)
  local listToSet = Get(list)
  listToSet[key] = value
end

/**
 * Adds a value to a list.
 *
 * @param list The list to add a value to.
 * @param value The value to insert.
 */
function Add(list, value)
  local listToAdd = Get(list)
  table.insert(listToAdd, value)
end
