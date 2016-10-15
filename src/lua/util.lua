
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

