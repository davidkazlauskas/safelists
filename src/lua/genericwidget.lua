
GenericWidget = {
    __index = {
        putOn = function(strongMsg)
            local res = {
                messageable = strongMsg,
                nodeCache = {}
            }
            setmetatable(res,GenericWidget)
            return res
        end
    }
}

