
// Outputs functions and stuff in wiki format

OUTPUT = ""

local function GetFunctions( tab )
  local functions = {}

  tab = tab or {}
  for k, v in pairs( tab ) do
    if ( type(v) == "function" ) then
      table.insert( functions, tostring(k) )
    end
  end

  table.sort( functions )
  return functions
end


local function DoLibrary( name )
  local firstchar = string.sub(name, 1, 1)
  local isclass = (firstchar == string.upper(firstchar))

  if ( type(_G[ name ]) != "table" ) then
    Msg("Error: _G["..name.."] is not a table!\n")
  end

  func = GetFunctions( _G[ name ] )

  if(isclass) then
    OUTPUT = OUTPUT .. "\n==[["..name.."]] ([[Object]])==\n"
  else
    OUTPUT = OUTPUT .. "\n==[["..name.."]] ([[Library]])==\n"
  end

  for k, v in pairs( func ) do
    OUTPUT = OUTPUT .. "* [["..name.."]].[["..name.."."..v.."|"..v.."]]\n"
  end
end


local Ignores = { "_G", "_R" }
local t ={}

for k, v in pairs(_G) do
  if ( type(v) == "table" && type(k) == "string" && !table.HasValue( Ignores, k ) ) then
    table.insert( t, tostring(k) )
  end
end

table.sort( t )
for k, v in pairs( t ) do
  Msg("Library: "..v.."\n")
  DoLibrary( v )
end


file.Write( "ServerFunctions.txt", OUTPUT )
