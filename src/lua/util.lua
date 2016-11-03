
function enumerateTable(table)
    local res = {}
    local index = 0
    for k,v in pairs(table) do
        index = index + 1
        res[index] = v
    end
    return res
end

function roundFloatStr(number,decimals)
    return string.format(
        "%." .. (decimals or 0) .. "f",
        number)
end

function humanReadableBytes(number)
    if (number >= 1024 * 1024) then
        return roundFloatStr((number / (1024 * 1024)),2) .. "MB"
    elseif (number >= 1024) then
        return roundFloatStr((number / 1024),2) .. "KB"
    else
        return number .. " bytes"
    end
end

function whole(number)
    assert( type(number) == "number", "Not number passed." )
    local res = string.format("%.0f",number)
    --print("In: |" .. number .. "|" .. res .. "|")
    return res
end

-- merge tables, new table overrides old
function mergeTables(tableOld,tableNew)
    local res = {}

    for k,v in pairs(tableOld) do
        res[k] = v
    end

    for k,v in pairs(tableNew) do
        res[k] = v
    end

    return res
end

function byteBelongsToHex(c)
    if (c >= 48 and c <= 57) then
        return true
    end

    if (c >= 65 and c <= 70) then
        return true
    end

    if (c >= 97 and c <= 102) then
        return true
    end

    return false
end

function isValidFilename(str)
    return string.match(
        str,'^[%w. _-]+$'
    ) ~= nil
end

function isValidDumbHash256(str)
    if (#str ~= 64) then
        return false
    end

    for i = 1, #str do
        local c = str:byte(i)
        if (not byteBelongsToHex(c)) then
            return false
        end
    end

    return true
end

-- LUA HAS NO SPLIT STRING FUNCTION? ARE YOU SERIOUS?
function string:split(delimiter)
  local result = { }
  local from = 1
  local delim_from, delim_to = string.find( self, delimiter, from )
  while delim_from do
    table.insert( result, string.sub( self, from , delim_from-1 ) )
    from = delim_to + 1
    delim_from, delim_to = string.find( self, delimiter, from )
  end
  table.insert( result, string.sub( self, from ) )
  return result
end

function trimString(arg)
  return string.match(arg,"^%s*(.-)%s*$")
end

function string:ends(tail)
    return tail == '' or
        string.sub(self,-string.len(tail)) == tail
end

function arrayBranch(value,func)
    return {
        value=value,
        func=func
    }
end

function arraySwitch(value,table,...)
    local branches = {...}
    local toFind = table[value]
    for k,v in pairs(branches) do
        if v.value == toFind then
            return v.func()
        end
    end
end

-- make function callable as a coroutine
function instrument(func)
    return function(...)
        local co = coroutine.create(func)
        return co, coroutine.resume(co,table.unpack({...}))
    end
end

-- TODO: we might need keys sometime?
function resumerCallbackValues(corout)
    return function(out)
        coroutine.resume(corout,out:values())
    end
end

function resumerCallback(corout)
    return function(...)
        coroutine.resume(corout,table.unpack({...}))
    end
end

function resumerCallbackWBranch(branch,corout)
    return function(...)
        coroutine.resume(corout,branch,table.unpack({...}))
    end
end

function resumerCallbackSwitch(corout,label)
    return function()
        coroutine.resume(corout,label)
    end
end
