
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

