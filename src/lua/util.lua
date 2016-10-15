
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
